#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Patch the media_id / version / base_version fields of the XEX2 EXECUTION_INFO
optional header (devkit builds leave them 0x00000000; retail titles carry a
non-zero media id + a version/build number).

Usage:
    python3 patch_media_id.py <input.xex> [-o output.xex] [--media-id 0x...] [--version N]

If -o is omitted the XEX is patched in place.
"""
import struct, sys, argparse

EXECUTION_INFO = 0x00040006

def be(d, o):
    return struct.unpack(">I", d[o:o+4])[0]

def find_exec_offset(d):
    if d[0:4] != b"XEX2":
        sys.exit("not a XEX2 file")
    n = be(d, 0x14)
    for i in range(n):
        o = 0x18 + i * 8
        k, v = be(d, o), be(d, o + 4)
        if k == EXECUTION_INFO:
            return v
    sys.exit("EXECUTION_INFO header not found")

def patch(path, out, media_id, version):
    d = bytearray(open(path, "rb").read())
    e = find_exec_offset(bytes(d))
    base_version = version
    # xex2 ExecutionInfo: media_id @0, version @4, base_version @8, title_id @12
    d[e + 0:e + 4] = struct.pack(">I", media_id)
    d[e + 4:e + 8] = struct.pack(">I", version)
    d[e + 8:e + 12] = struct.pack(">I", base_version)
    # report the (now unchanged) title_id @ +12
    title_id = be(d, e + 12)
    open(out, "wb").write(d)
    print("patched '%s' -> '%s'" % (path, out))
    print("  media_id     = 0x%08X" % media_id)
    print("  version      = 0x%08X" % version)
    print("  base_version = 0x%08X" % base_version)
    print("  title_id     = 0x%08X (unchanged)" % title_id)

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("xex")
    ap.add_argument("-o", "--output")
    ap.add_argument("--media-id", default="0x4B1D5EAF")
    ap.add_argument("--version", default="0x1")
    a = ap.parse_args()
    out = a.output or a.xex
    patch(a.xex, out, int(a.media_id, 16), int(a.version, 16))

if __name__ == "__main__":
    main()