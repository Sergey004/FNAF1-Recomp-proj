#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
fix_pak_sounds.py — v2.6 sound fixer for fnaf1.pak (Xbox 360 recomp).

The pak container is big-endian (header fields BE32, texture blobs stored
with byte-swapped DXT words). Depending on which tool wrote a given pak,
the sound payloads inside may be:

  1) headerless LITTLE-endian PCM16  (fine)
  2) headerless BIG-endian PCM16     (played as LE = pure noise)
  3) complete RIFF/WAVE files        (44..172-byte headers; some of the
                                      original WAVs have LIST/INFO chunks)
  4) RIFF with byte-swapped payloads
  5) 8-bit PCM (XSCREAM) presented as 16-bit (noise)

This tool rewrites every sound entry into case 1 (raw LE PCM16), keeping
all texture/font blobs byte-identical, and prints a per-sound report.

Usage:
  python fix_pak_sounds.py fnaf1.pak                 # fix in place (writes fnaf1.pak.fixed)
  python fix_pak_sounds.py fnaf1.pak -o out.pak      # explicit output
  python fix_pak_sounds.py fnaf1.pak --force-swap    # treat ALL PCM as big-endian
  python fix_pak_sounds.py fnaf1.pak --force-native  # treat ALL PCM as little-endian
  python fix_pak_sounds.py fnaf1.pak --sounds-dir D  # replace blobs from dumped WAVs
                                                     # (ctfak-cpp "Sound Dumper" output)
"""
import os
import re
import struct
import sys

MAGIC = 0x464E4146  # 'FNAF'


# ----------------------------------------------------------------------
# low-level readers
# ----------------------------------------------------------------------
def be32(b, o):
    return struct.unpack_from('>I', b, o)[0]


def le32(b, o):
    return struct.unpack_from('<I', b, o)[0]


def le16(b, o):
    return struct.unpack_from('<H', b, o)[0]


def cstr(b, o):
    e = b.find(b'\x00', o)
    if e < 0:
        e = len(b)
    return b[o:e].decode('latin-1')


def swap16_in_place(ba):
    n = len(ba) // 2
    a = bytearray(ba)
    a[0:n * 2] = struct.pack('<%dH' % n, *struct.unpack('>%dH' % n, bytes(a[0:n * 2])))
    return bytes(a)


# ----------------------------------------------------------------------
# RIFF parsing (LE sizes first, BE as fallback)
# ----------------------------------------------------------------------
def riff_parse(b):
    if len(b) < 64 or b[0:4] != b'RIFF' or b[8:12] != b'WAVE':
        return None
    pos, fmt, data = 12, None, None
    while pos + 8 <= len(b):
        cid = b[pos:pos + 4]
        maxsz = len(b) - pos - 8
        sz = le32(b, pos + 4)
        if sz > maxsz:
            szBE = be32(b, pos + 4)
            if szBE <= maxsz:
                sz = szBE
            else:
                return None
        if cid == b'fmt ' and sz >= 16:
            tag, ch, rate = struct.unpack_from('<HHI', b, pos + 8)
            bits = le16(b, pos + 8 + 14)
            if tag == 0xFFFE and sz >= 40:          # extensible: real tag in SubFormat
                tag = le16(b, pos + 8 + 24)
            fmt = (tag, ch, rate, bits)
        elif cid == b'data':
            off = pos + 8
            data = (off, min(sz, len(b) - off))
            break
        pos += 8 + sz + (sz & 1)
    if not fmt or not data:
        return None
    tag, ch, rate, bits = fmt
    if tag != 1 or not (1 <= ch <= 2) or not (3000 <= rate <= 96000) or bits not in (8, 16):
        return None
    return {'tag': tag, 'ch': ch, 'rate': rate, 'bits': bits,
            'off': data[0], 'size': data[1]}


# ----------------------------------------------------------------------
# byte-order probe (same as the runtime)
#
# Real audio stores the waveform COARSE value in the high byte, so the
# WRONG reading always inflates the mean amplitude several-fold. Read
# both ways; the smaller one is the true order. Measured on the real
# FNaF1 bank: 50/51 sounds separate by 3.1x..88x (XSCREAM2 near-ties and
# abstains; the pak-wide majority covers it).
# ----------------------------------------------------------------------
def pcm_byte_order_vote(pcm):
    """returns 1 = looks BE, 0 = looks LE, -1 = abstain"""
    n = len(pcm) // 2
    if n < 512:
        return -1
    a = struct.unpack('<%dH' % n, pcm[:n * 2])
    b = struct.unpack('>%dH' % n, pcm[:n * 2])
    scan = min(n, 2000000)
    sle = sbe = 0
    for i in range(scan):
        v = a[i]
        sle += (65536 - v) if v >= 32768 else v
        w = b[i]
        sbe += (65536 - w) if w >= 32768 else w
    le = sle / float(scan)
    be = sbe / float(scan)
    if le < 4.0 and be < 4.0:
        return -1                      # digital silence
    if be < le * 0.8:
        return 1                       # BE reading much quieter -> stored BE
    if le < be * 0.8:
        return 0                       # LE reading much quieter -> stored LE
    return -1                          # near-tie


def expand8(pcm):
    out = bytearray(len(pcm) * 2)
    for i, v in enumerate(pcm):
        s = (v - 128) << 8
        struct.pack_into('<h', out, i * 2, s if s < 32767 else 32767)
    return bytes(out)


def is_literal_noise(name):
    return 'static' in name.lower()


XSCREAM_SIZES = (115316, 115360)  # raw u8 payload / whole-file


def is_known_8bit(name, size):
    return 'XSCREAM' in name.upper() and size in XSCREAM_SIZES


# ----------------------------------------------------------------------
# pak model
# ----------------------------------------------------------------------
class Pak:
    def __init__(self, path):
        self.path = path
        self.raw = open(path, 'rb').read()
        r = self.raw
        if len(r) < 32 or be32(r, 0) != MAGIC:
            raise SystemExit('not a fnaf1.pak (bad magic)')
        self.version = be32(r, 4)
        self.num_tex = be32(r, 8)
        self.num_snd = be32(r, 12)
        self.num_mus = be32(r, 16)
        self.num_fnt = be32(r, 20)
        self.strings_off = be32(r, 24)
        self.data_off = be32(r, 28)

        p = 32
        self.tex = []
        for _ in range(self.num_tex):
            self.tex.append({
                'nameOff': be32(r, p), 'dataOff': be32(r, p + 4),
                'dataSize': be32(r, p + 8), 'fmt': be32(r, p + 12),
                'origW': be32(r, p + 16), 'origH': be32(r, p + 20),
                'alignW': be32(r, p + 24), 'alignH': be32(r, p + 28),
                'mip': be32(r, p + 32),
            })
            p += 36
        self.snd = []
        for _ in range(self.num_snd):
            self.snd.append({
                'nameOff': be32(r, p), 'dataOff': be32(r, p + 4),
                'dataSize': be32(r, p + 8), 'fmt': be32(r, p + 12),
                'rate': be32(r, p + 16), 'ch': be32(r, p + 20),
                'loopS': be32(r, p + 24), 'loopE': be32(r, p + 28),
            })
            p += 32
        self.fnt = []
        for _ in range(self.num_fnt):
            self.fnt.append({
                'nameOff': be32(r, p), 'atlasOff': be32(r, p + 4),
                'glyphFirst': be32(r, p + 8), 'glyphCount': be32(r, p + 12),
                'pixelHeight': be32(r, p + 16), 'ascent': be32(r, p + 20),
                'lineHeight': be32(r, p + 24), 'references': be32(r, p + 28),
                'metricsOff': be32(r, p + 32), 'metricsSize': be32(r, p + 36),
            })
            p += 40

        for t in self.tex:
            t['name'] = cstr(r, t['nameOff'])
            t['data'] = bytes(r[t['dataOff']:t['dataOff'] + t['dataSize']])
        for s in self.snd:
            s['name'] = cstr(r, s['nameOff'])
            s['data'] = bytes(r[s['dataOff']:s['dataOff'] + s['dataSize']])
        for f in self.fnt:
            f['name'] = cstr(r, f['nameOff'])
            f['atlas'] = cstr(r, f['atlasOff'])
            f['metrics'] = bytes(r[f['metricsOff']:f['metricsOff'] + f['metricsSize']])

    def write(self, path):
        num_tex, num_snd, num_fnt = len(self.tex), len(self.snd), len(self.fnt)
        strings_off = 32 + num_tex * 36 + num_snd * 32 + num_fnt * 40
        pool_len = 0
        for t in self.tex:
            pool_len += len(t['name']) + 1
        for s in self.snd:
            pool_len += len(s['name']) + 1
        for f in self.fnt:
            pool_len += len(f['name']) + 1 + len(f['atlas']) + 1
        data_off = (strings_off + pool_len + 15) & ~15

        pool = bytearray()
        blob = bytearray()
        tex_ent, snd_ent, fnt_ent = [], [], []

        for t in self.tex:
            off = data_off + len(blob)
            blob += t['data']
            n_at = strings_off + len(pool)
            pool += t['name'].encode('latin-1') + b'\x00'
            tex_ent.append((n_at, off, len(t['data']), t['fmt'],
                            t['origW'], t['origH'], t['alignW'], t['alignH'], t['mip']))
        for s in self.snd:
            off = data_off + len(blob)
            blob += s['data']
            n_at = strings_off + len(pool)
            pool += s['name'].encode('latin-1') + b'\x00'
            snd_ent.append((n_at, off, len(s['data']), s['fmt'],
                            s['rate'], s['ch'], s['loopS'], s['loopE']))
        for f in self.fnt:
            off = data_off + len(blob)
            blob += f['metrics']
            n_at = strings_off + len(pool)
            pool += f['name'].encode('latin-1') + b'\x00'
            a_at = strings_off + len(pool)
            pool += f['atlas'].encode('latin-1') + b'\x00'
            fnt_ent.append((n_at, a_at, f['glyphFirst'], f['glyphCount'],
                            f['pixelHeight'], f['ascent'], f['lineHeight'],
                            f['references'], off, len(f['metrics'])))

        out = bytearray()
        out += struct.pack('>8I', MAGIC, self.version, num_tex, num_snd,
                           self.num_mus, num_fnt, strings_off, data_off)
        for e in tex_ent:
            out += struct.pack('>9I', *e)
        for e in snd_ent:
            out += struct.pack('>8I', *e)
        for e in fnt_ent:
            out += struct.pack('>10I', *e)
        out += pool
        out += b'\x00' * (data_off - len(out))
        out += blob
        open(path, 'wb').write(bytes(out))


# ----------------------------------------------------------------------
# normalization
# ----------------------------------------------------------------------
def load_wav(path):
    b = open(path, 'rb').read()
    ri = riff_parse(b)
    if not ri:
        return None
    pcm = b[ri['off']:ri['off'] + ri['size']]
    if ri['bits'] == 8:
        pcm = expand8(pcm)
    return pcm, ri['rate'], ri['ch']


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return
    path = args[0]
    out = None
    force_swap = '--force-swap' in args
    force_native = '--force-native' in args
    sounds_dir = None
    if '-o' in args:
        out = args[args.index('-o') + 1]
    if '--sounds-dir' in args:
        sounds_dir = args[args.index('--sounds-dir') + 1]

    pak = Pak(path)
    print('fnaf1.pak: v%d, %d textures, %d sounds, %d fonts' %
          (pak.version, pak.num_tex, pak.num_snd, pak.num_fnt))

    # ---- optional: replace blobs straight from the dumped WAVs --------
    replaced = 0
    if sounds_dir:
        for s in pak.snd:
            base = s['name']
            for ext in ('.wav',):
                cand = os.path.join(sounds_dir, base + ext)
                if os.path.isfile(cand):
                    r = load_wav(cand)
                    if r:
                        s['data'], s['rate'], s['ch'] = r
                        s['fmt'] = 0
                        replaced += 1
                    break
        print('replaced from Sounds dir: %d' % replaced)

    # ---- pass 1: peel RIFF headers ------------------------------------
    for s in pak.snd:
        ri = riff_parse(s['data'])
        if ri:
            s['data'] = s['data'][ri['off']:ri['off'] + ri['size']]
            s['rate'], s['ch'], s['bits'] = ri['rate'], ri['ch'], ri['bits']
            s['riff'] = True
        else:
            s['bits'] = 16
            s['riff'] = False

    # ---- pass 2: byte-order vote ---------------------------------------
    votes = []
    for s in pak.snd:
        if s['fmt'] != 0 and s['fmt'] != 1:
            continue
        if s['bits'] != 16 or is_literal_noise(s['name']) or is_known_8bit(s['name'], len(s['data'])):
            continue
        votes.append((s['name'], pcm_byte_order_vote(s['data'])))
    n_swap = sum(1 for _, v in votes if v > 0)
    n_native = sum(1 for _, v in votes if v == 0)
    n_abstain = len(votes) - n_swap - n_native
    if force_swap:
        swap_all = True
    elif force_native:
        swap_all = False
    else:
        swap_all = n_swap > n_native and n_swap > 0
    print('byte-order vote: BE=%d LE=%d abstain=%d -> %s' %
          (n_swap, n_native, n_abstain, 'SWAP ALL' if swap_all else 'little-endian'))

    # ---- pass 3: fix -----------------------------------------------------
    vote_by_name = dict(votes)
    rows = []
    for s in pak.snd:
        old_size = len(s['data'])
        vt = vote_by_name.get(s['name'], -2)
        vote_s = {1: 'BE', 0: 'LE', -1: '-', -2: ''}[vt]
        if s['bits'] == 8 or is_known_8bit(s['name'], old_size):
            s['data'] = expand8(s['data'])
            mode = '8bit->16'
        elif s['bits'] != 16:
            mode = 'UNSUPPORTED(fmt=%d)' % s['fmt']
        else:
            if swap_all:
                s['data'] = swap16_in_place(s['data'])
                mode = 'BE->LE'
            else:
                mode = 'le'
        s['fmt'] = 0 if 'UNSUPPORTED' not in mode else 1
        rows.append((s['name'], 'RIFF' if s['riff'] else 'raw', mode + vote_s,
                     s['rate'], s['ch'], old_size, len(s['data'])))

    name_w = max(len(r[0]) for r in rows) + 1
    print('\n%-*s %-5s %-16s %7s %3s %10s -> %10s' %
          (name_w, 'sound', 'src', 'action+vote', 'rate', 'ch', 'old', 'new'))
    for r in rows:
        print('%-*s %-5s %-16s %7d %3d %10d -> %10d' %
              (name_w, r[0], r[1], r[2], r[3], r[4], r[5], r[6]))

    if out is None:
        out = path + '.fixed'
    pak.write(out)
    print('\nwritten: %s (%d bytes)' % (out, len(open(out, 'rb').read())))
    print('REPLACE the old fnaf1.pak with it (keep a backup!) and rebuild nothing -')
    print('the v2.6 runtime also normalizes on the fly, this file is just the clean form.')


if __name__ == '__main__':
    main()
