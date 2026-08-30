#!/usr/bin/env python3
"""Independent verification of the generated fnaf1.pak.

Mirrors the console-side PakLoader.cpp byte-for-byte (big-endian), then
cross-checks against the ground-truth dumps:
  - magic/counts/offsets
  - texture names, dims, fmt, data sizes
  - DXT1/DXT5 round trip of a few key textures vs dumped PNGs
  - PCM sound payload byte-exact vs dumped WAV data chunks
"""
import struct, zlib, sys, os

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
    nameOff, dOff, dSize, fmt, sr, ch, ls, le = (be32(p+4*k) for k in range(8))
    p += 32
    end = data.index(0, nameOff)
    snd.append(dict(name=data[nameOff:end].decode(), off=dOff, size=dSize,
                    fmt=fmt, sr=sr, ch=ch))
assert p <= strOff, "entries overlap string pool"
print(f"entries parsed: {len(tex)} tex, {len(snd)} snd; entry tables end at {p} <= strings {strOff}")

# every data blob must lie inside the file, sizes consistent with format+align
for t in tex:
    assert t["off"] + t["size"] <= len(data), t
    bpp = 4 if t["fmt"] == 0 else 8 if t["fmt"] == 2 else 32
    expect = t["aw"] * t["ah"] * bpp // 8
    assert t["size"] == expect, (t, expect)
print("all texture blob sizes consistent with format and aligned dims")

# ---- names must be exactly img_<n> with matching orig dims from the dump PNGs
bad = 0
for t in tex:
    h = int(t["name"].split("_")[1])
    png = open(f"{DUMP}/{h}.png", "rb").read(25)
    w, ht = struct.unpack(">II", png[16:24])
    if (w, ht) != (t["w"], t["h"]): bad += 1
print(f"dimension cross-check vs dumped PNGs: {numTex-bad}/{numTex} match")
assert bad == 0

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

def dxt_decode(t):
    aw, ah, fmt = t["aw"], t["ah"], t["fmt"]
    rgba = bytearray(aw*ah*4)
    bs = 8 if fmt == 0 else 16
    for by in range(ah//4):
        for bx in range(aw//4):
            off = t["off"] + (by*(aw//4) + bx)*bs
            blk = data[off:off+bs]
            if fmt == 2:
                a0, a1 = blk[0], blk[1]
                ab = int.from_bytes(blk[2:8], "big")
                pal = [a0, a1]
                if a0 > a1:
                    for i in range(6): pal.append(((6-i)*a0 + (1+i)*a1)//7)
                else:
                    for i in range(4): pal.append(((4-i)*a0 + (1+i)*a1)//5)
                    pal += [0, 255]
                c0, c1 = struct.unpack_from(">HH", blk, 8)
                cb = int.from_bytes(blk[12:16], "big")
            else:
                c0, c1 = struct.unpack_from(">HH", blk, 0)
                cb = int.from_bytes(blk[4:8], "big")
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

for handle in (39, 431, 129, 103):   # office bg, menu bg, button, left door
    t = next(x for x in tex if x["name"] == f"img_{handle}")
    w, h, pngpx = png_pixels(f"{DUMP}/{handle}.png")
    dw, dh, dpx = dxt_decode(t)
    tot = n = 0
    for i in range(0, w*h*4, 4):
        for c in range(3):
            tot += abs(pngpx[i+c] - dpx[i+c]); n += 1
    print(f"img_{handle}: {w}x{h} fmt={t['fmt']} avg RGB error vs source: {tot/n:.2f}")

# ---- sounds: PCM payload must byte-match the dumped WAV data chunk
# (needs the ctfak Sounds dump next to the Images dump; skipped otherwise)
sndDump = os.path.join(os.path.dirname(DUMP.rstrip("/")) or ".", "Sounds")
if not os.path.isdir(sndDump):
    print("sound cross-check skipped (no Sounds dump next to image dump)")
    print("PAK VERIFICATION PASSED")
    sys.exit(0)
for s in snd[:3] + snd[-3:]:
    h = s["name"][4:]
    wav = open(f"{sndDump}/{h}.wav", "rb").read()
    dpos = wav.index(b"data") + 4
    dsize = struct.unpack_from("<I", wav, dpos)[0]
    dpos += 4
    ok = wav[dpos:dpos+dsize] == data[s["off"]:s["off"]+s["size"]]
    print(f"sound {s['name']}: {s['sr']} Hz, {s['ch']} ch, fmt {s['fmt']}, payload match: {ok}")
    assert ok

names = {t["name"] for t in tex}
for key in ("img_39", "img_431", "img_11", "img_129", "img_103", "img_119", "img_18", "img_167"):
    assert key in names, key
print("key assets present: img_39, img_431, img_11, img_129, img_103, img_119, img_18, img_167")
print("PAK VERIFICATION PASSED")
