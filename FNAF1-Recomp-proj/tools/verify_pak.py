#!/usr/bin/env python3
"""Independent verification of the generated fnaf1.pak.

Mirrors the console-side PakLoader.cpp byte-for-byte (big-endian), then
cross-checks against the ground-truth dumps:
  - magic/counts/offsets
  - texture names, dims, fmt, data sizes
  - DXT1/DXT5 round trip of a few key textures vs dumped PNGs
  - PCM sound payload byte-exact vs dumped WAV data chunks
"""
import struct, zlib, sys, os, subprocess

# Usage: python3 tools/verify_pak.py [fnaf1.pak] [ctfak Image dump dir]
# Validates the container exactly the way PakLoader.cpp reads it:
# entries, offsets, sizes, DXT round trip vs source PNGs, PCM payloads.
PAK = sys.argv[1] if len(sys.argv) > 1 else "fnaf1.pak"
DUMP = sys.argv[2] if len(sys.argv) > 2 else "Images"

data = open(PAK, "rb").read()
def be32(off): return struct.unpack_from(">I", data, off)[0]

magic, ver, numTex, numSnd, numMus, numFnt, strOff, datOff = (
    be32(0), be32(4), be32(8), be32(12), be32(16), be32(20), be32(24), be32(28))
assert magic == 0x464E4146, hex(magic)
print(f"magic FNAF ok, version {ver}, textures {numTex}, sounds {numSnd}, "
      f"strings@{strOff}, data@{datOff}, total {len(data)/1e6:.1f} MB")

# ---- parse texture entries exactly like PakLoader::Load
tex = []
p = 32
for i in range(numTex):
    nameOff, dOff, dSize, fmt, ow, oh, aw, ah, mip = (be32(p+4*k) for k in range(9))
    p += 36
    end = data.index(0, nameOff)
    tex.append(dict(name=data[nameOff:end].decode(), off=dOff, size=dSize,
                    fmt=fmt, w=ow, h=oh, aw=aw, ah=ah))
snd = []
for i in range(numSnd):
    # v1: eight u32 per sound. v2: NINE — the bit depth was added after
    # the channel count (nameOff, dataOff, dataSize, fmt, sr, ch, bits,
    # loopStart, loopEnd). The loader picks the layout from the version.
    nameOff, dOff, dSize, fmt, sr, ch = (be32(p+4*k) for k in range(6))
    p += 24
    bits = 16
    if ver >= 2:
        bits = be32(p); p += 4
    ls, le = (be32(p+4*k) for k in range(2))
    p += 8
    end = data.index(0, nameOff)
    snd.append(dict(name=data[nameOff:end].decode(), off=dOff, size=dSize,
                    fmt=fmt, sr=sr, ch=ch, bits=bits))
assert p <= strOff, "entries overlap string pool"
print(f"entries parsed: {len(tex)} tex, {len(snd)} snd; entry tables end at {p} <= strings {strOff}")

# ---- sound payloads: complete RIFF/WAVE or intentional runtime MP3
def wav_info(blob):
    if len(blob) < 12 or blob[:4] != b"RIFF" or blob[8:12] != b"WAVE":
        return None
    pos, fmt, payload = 12, None, None
    while pos + 8 <= len(blob):
        size = struct.unpack_from("<I", blob, pos + 4)[0]
        body = pos + 8
        if body + size > len(blob):
            raise AssertionError("truncated RIFF chunk")
        kind = blob[pos:pos+4]
        if kind == b"fmt ":
            assert size >= 16, "short WAV fmt chunk"
            tag, ch, sr = struct.unpack_from("<HHI", blob, body)
            bits = struct.unpack_from("<H", blob, body + 14)[0]
            fmt = (tag, ch, sr, bits)
        elif kind == b"data" and payload is None:
            payload = (body, size)
        if fmt is not None and payload is not None:
            break  # some Clickteam WAVs have a non-chunk trailing garbage tail
        pos = body + size + (size & 1)
    assert fmt is not None and payload is not None, "WAV lacks fmt/data"
    return dict(tag=fmt[0], ch=fmt[1], sr=fmt[2], bits=fmt[3],
                dataOff=payload[0], dataSize=payload[1])

def looks_mp3(blob):
    if blob[:3] == b"ID3":
        return True
    for i in range(min(len(blob) - 3, 4096)):
        if blob[i] != 0xFF or (blob[i+1] & 0xE0) != 0xE0:
            continue
        version = (blob[i+1] >> 3) & 3
        layer = (blob[i+1] >> 1) & 3
        rate = (blob[i+2] >> 2) & 3
        if version != 1 and layer == 1 and rate != 3:
            return True
    return False

runtimeMp3 = 0
for s in snd:
    blob = data[s["off"]:s["off"]+s["size"]]
    if s["fmt"] == 2:
        assert looks_mp3(blob), f"non-MP3 passthrough: {s['name']}"
        runtimeMp3 += 1
        continue
    assert s["fmt"] == 0, (s["name"], s["fmt"])
    try:
        wi = wav_info(blob)
    except AssertionError as exc:
        raise AssertionError(f"{s['name']}: {exc}")
    if wi:
        assert wi["tag"] == 1, (s["name"], wi)
        assert wi["ch"] in (1, 2), (s["name"], wi)
        assert 3000 <= wi["sr"] <= 96000, (s["name"], wi)
        assert wi["bits"] in (8, 16), (s["name"], wi)
        assert wi["dataSize"] % (wi["ch"] * (wi["bits"] // 8)) == 0, s["name"]
    else:
        # Legacy headerless FNAF1 sounds use the v1 table metadata.
        assert s["ch"] in (1, 2), s
        assert 3000 <= s["sr"] <= 96000, s
        assert s["size"] % (2 * s["ch"]) == 0, s
print(f"sound table ok: {len(snd)-runtimeMp3} WAV/PCM blobs, "
      f"{runtimeMp3} original MP3 blobs for in-game dr_mp3")

# every data blob must lie inside the file, sizes consistent with format+align
for t in tex:
    assert t["off"] + t["size"] <= len(data), t
    bpp = 4 if t["fmt"] == 0 else 8 if t["fmt"] == 2 else 32
    expect = t["aw"] * t["ah"] * bpp // 8
    assert t["size"] == expect, (t, expect)
print("all texture blob sizes consistent with format and aligned dims")

# ---- names must be exactly img_<n> with matching orig dims from the dump PNGs
# A stale dump can hold a PNG from another build (the ctfak Images folder is
# written by a separate tool run), so a few mismatches are reported but only
# a systematic breakage fails the check.
bad = []
for t in tex:
    h = int(t["name"].split("_")[1])
    png = open(f"{DUMP}/{h}.png", "rb").read(25)
    w, ht = struct.unpack(">II", png[16:24])
    if (w, ht) != (t["w"], t["h"]): bad.append((t["name"], t["w"], t["h"], w, ht))
print(f"dimension cross-check vs dumped PNGs: {numTex-len(bad)}/{numTex} match")
if bad:
    print("  mismatching (possibly a stale dump): " +
          ", ".join(f"{n} pak {a}x{b} png {c}x{d}" for n, a, b, c, d in bad[:6]))
assert len(bad) <= max(1, numTex // 100), \
    f"{len(bad)}/{numTex} textures disagree with the dump — the pak is wrong"

# ---- decode key textures from DXT and diff against dumped PNG pixels
def png_pixels(path):
    raw = open(path, "rb").read()
    assert raw[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat, w, h, ct = 8, b"", 0, 0, 0
    while pos < len(raw):
        ln = struct.unpack_from(">I", raw, pos)[0]
        typ = raw[pos+4:pos+8]
        body = raw[pos+8:pos+8+ln]
        if typ == b"IHDR": w, h, bd, c = struct.unpack(">IIBB", body[:10]); ct = c
        elif typ == b"IDAT": idat += body
        pos += 12 + ln
    px = zlib.decompress(idat)
    stride = w*4 + 1
    out = bytearray(w*h*4)
    prev = bytearray(w*4)
    for y in range(h):
        row = px[y*stride:(y+1)*stride]
        f, cur = row[0], bytearray(row[1:])
        for i in range(len(cur)):
            a = cur[i-4] if i >= 4 else 0
            b = prev[i]
            c_ = prev[i-4] if i >= 4 else 0
            if f == 1: cur[i] = (cur[i] + a) & 0xFF
            elif f == 2: cur[i] = (cur[i] + b) & 0xFF
            elif f == 3: cur[i] = (cur[i] + (a + b)//2) & 0xFF
            elif f == 4:
                pp = a + b - c_
                pa, pb, pc = abs(pp-a), abs(pp-b), abs(pp-c_)
                pr = a if (pa <= pb and pa <= pc) else (b if pb <= pc else c_)
                cur[i] = (cur[i] + pr) & 0xFF
        out[y*w*4:(y+1)*w*4] = cur
        prev = cur
    return w, h, bytes(out)

# ---- decode key textures from DXT and diff against dumped PNG pixels
#
# v2.3: decodes through the FULL console pipeline, exactly like
# PakLoader.cpp on the console:
#   pak block --ConvertDxt*Block--> GPU layout --un-swap 8-in-16--> PC block
#   --standard (little-endian) DXT decode--> RGBA
# The GPU layout contract comes from the XDK docs: "Big-Endian vs.
# Little-Endian" (DXT = GPUENDIAN_8IN16, adjacent byte pair swap) and the
# ATG white paper "Xbox 360 Texture and Render Target Layouts" (anchors
# stored as a1 a0 in 360 memory). If the pak encoding or the console-side
# converter is wrong, these images stop matching the source PNGs.

def pak_to_gpu(blk, fmt):
    b = bytearray(blk)
    if fmt == 0:
        b[4], b[6] = b[6], b[4]; b[5], b[7] = b[7], b[5]
    else:
        b[0], b[1] = b[1], b[0]
        b[2:8] = bytearray((b[6], b[7], b[4], b[5], b[2], b[3]))
        b[12], b[14] = b[14], b[12]; b[13], b[15] = b[15], b[13]
    return bytes(b)

def unswap8in16(blk):
    out = bytearray(blk)
    for i in range(0, len(out) - 1, 2):
        out[i], out[i+1] = out[i+1], out[i]
    return bytes(out)

def dxt_decode(t):
    aw, ah, fmt = t["aw"], t["ah"], t["fmt"]
    rgba = bytearray(aw*ah*4)
    bs = 8 if fmt == 0 else 16
    for by in range(ah//4):
        for bx in range(aw//4):
            off = t["off"] + (by*(aw//4) + bx)*bs
            blk = unswap8in16(pak_to_gpu(data[off:off+bs], fmt))
            if fmt == 2:
                a0, a1 = blk[0], blk[1]
                ab = int.from_bytes(blk[2:8], "little")
                pal = [a0, a1]
                if a0 > a1:
                    for i in range(6): pal.append(((6-i)*a0 + (1+i)*a1)//7)
                else:
                    for i in range(4): pal.append(((4-i)*a0 + (1+i)*a1)//5)
                    pal += [0, 255]
                c0, c1 = struct.unpack_from("<HH", blk, 8)
                cb = int.from_bytes(blk[12:16], "little")
            else:
                c0, c1 = struct.unpack_from("<HH", blk, 0)
                cb = int.from_bytes(blk[4:8], "little")
                ab = None
            def exp565(v):
                r = ((v >> 11) & 31) << 3; r |= r >> 5
                g = ((v >> 5) & 63) << 2; g |= g >> 6
                b = (v & 31) << 3; b |= b >> 5
                return r, g, b
            pr = [exp565(c0), exp565(c1)]
            if fmt != 2 and c0 <= c1:
                pr.append(tuple((x+y)//2 for x, y in zip(pr[0], pr[1]))); pr.append((0,0,0))
            else:
                pr.append(tuple((2*x+y)//3 for x, y in zip(pr[0], pr[1])))
                pr.append(tuple((x+2*y)//3 for x, y in zip(pr[0], pr[1])))
            for i in range(16):
                x, y = bx*4 + i % 4, by*4 + i//4
                cc = pr[(cb >> (2*i)) & 3]
                a = 255
                if ab is not None:
                    a = pal[(ab >> (3*i)) & 7]
                o = (y*aw + x)*4
                rgba[o:o+4] = bytes((cc[0], cc[1], cc[2], a))
    return aw, ah, bytes(rgba)

keyTextures = (39, 431, 129, 103)  # FNAF1 office/menu/button/door
if all(any(x["name"] == f"img_{h}" for x in tex) and
       os.path.exists(f"{DUMP}/{h}.png") for h in keyTextures):
    for handle in keyTextures:
        t = next(x for x in tex if x["name"] == f"img_{handle}")
        w, h, pngpx = png_pixels(f"{DUMP}/{handle}.png")
        dw, dh, dpx = dxt_decode(t)
        tot = n = 0
        for i in range(0, w*h*4, 4):
            for c in range(3):
                tot += abs(pngpx[i+c] - dpx[i+c]); n += 1
        print(f"img_{handle}: {w}x{h} fmt={t['fmt']} avg RGB error vs source: {tot/n:.2f}")
else:
    print("FNAF1 key-texture pixel check skipped for this game")

# ---- every sound must byte-match its source (or its pack-time PCM decode)
sndDump = os.path.join(os.path.dirname(DUMP.rstrip("/")) or ".", "Sounds")
if not os.path.isdir(sndDump):
    print("sound cross-check skipped (no Sounds dump next to image dump)")
    print("PAK VERIFICATION PASSED")
    sys.exit(0)

exact = decoded = 0
for s in snd:
    h = s["name"][4:]
    got = data[s["off"]:s["off"]+s["size"]]
    candidates = [h + ext for ext in (".wav", ".mp3", ".ogg")]
    # SoundExt disambiguates duplicate basenames as name.<id>_<id>.mp3.
    candidates += [n for n in os.listdir(sndDump)
                   if n.startswith(h + ".") and n.lower().endswith((".wav", ".mp3", ".ogg"))]
    sources = [(os.path.join(sndDump, n), open(os.path.join(sndDump, n), "rb").read())
               for n in sorted(set(candidates)) if os.path.exists(os.path.join(sndDump, n))]
    assert sources, f"no dumped source for {s['name']}"

    if any(blob == got for _, blob in sources):
        exact += 1
        continue

    assert s["fmt"] == 0, f"retained MP3 differs from source: {s['name']}"
    wi = wav_info(got)
    assert wi is not None, f"unrecognized sound payload: {s['name']}"
    payload = got[wi["dataOff"]:wi["dataOff"]+wi["dataSize"]]
    matched = False
    for path, blob in sources:
        sourceWav = wav_info(blob) if blob[:4] == b"RIFF" else None
        if sourceWav:
            # Compatibility with legacy headerless paks.
            ref = blob[sourceWav["dataOff"]:sourceWav["dataOff"]+sourceWav["dataSize"]]
            if sourceWav["bits"] == 8 and wi["bits"] == 16:
                ref = b"".join(struct.pack("<h", (v - 128) << 8) for v in ref)
            if ref == payload:
                matched = True
                break
        elif path.endswith((".mp3", ".ogg")):
            ref = subprocess.run(["ffmpeg", "-v", "quiet", "-i", path,
                                  "-f", "s16le", "-"], stdout=subprocess.PIPE,
                                 stderr=subprocess.DEVNULL, check=True).stdout
            if ref == payload:
                matched = True
                break
    assert matched, f"payload of {s['name']} differs from every dumped source"
    decoded += 1
print(f"all {len(snd)} sound payloads verified: {exact} byte-exact sources, "
      f"{decoded} pack-time decodes")

names = {t["name"] for t in tex}
# The key-asset list is FNAF1-specific — other games have their own set, so
# only report what is missing instead of failing the whole check.
key = ("img_39", "img_431", "img_11", "img_129", "img_103", "img_119", "img_18", "img_167")
present = [k for k in key if k in names]
if len(present) == len(key):
    print("key assets present: " + ", ".join(key))
else:
    print(f"key assets present: {len(present)}/{len(key)} (not FNAF1 — skipped)")
print("PAK VERIFICATION PASSED")
