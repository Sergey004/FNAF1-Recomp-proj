#!/usr/bin/env python3
"""Generate the FNAF3 jumpscare tables (the two death animations).

The dump's "jumpscare" (objInfo 128) and "jumpscare 2" (objInfo 223) each
carry a 41-cell animation at speed 60 (60 * 0.6 = 36 fps). The cells have
their own sizes AND their own hotspots, so the draw origin of every cell is
the create position minus that cell's hotspot.

Inputs (from the ctfak dump):
  JSON/application.json    the two objects' animation cell lists
  Images/hotspots.txt      "<handle> <w> <h> <hotX> <hotY>" per image

The create positions come from the events dump ("create x182 \"jumpscare\"
at (0,0) rel \"00 anchor 2\"" / "at (1024,0) rel ...") — the anchors sit at
the world origin, so the instance points are (0,0) and (1024,0).

Usage: python3 tools/gen_scares.py "<dumps>/Five Nights at Freddys 3" include/FNaF3Scares.h
"""
import json
import os
import sys

DUMPS = sys.argv[1]
OUT = sys.argv[2]

data = json.load(open(os.path.join(DUMPS, "JSON", "application.json"), encoding="utf-8"))
objs = {o["handle"]: o for o in data["objects"]}

hot = {}
with open(os.path.join(DUMPS, "Images", "hotspots.txt"), encoding="utf-8") as f:
    for line in f:
        p = line.split()
        if len(p) == 5:
            hot[int(p[0])] = (int(p[1]), int(p[2]), int(p[3]), int(p[4]))


def cells_of(handle):
    common = objs[handle].get("common") or {}
    out = []
    for _dirn, seqs in (common.get("animations") or {}).items():
        for _seq, fr in seqs.items():
            out = fr.get("frames") or []
    return out


# the instance points (the create positions relative to the "00 anchor 2",
# which sits at the world origin)
SCARES = [
    ("FNaF3Scare1", 128, 0, 0),
    ("FNaF3Scare2", 223, 1024, 0),
]

out = []
out.append("/**")
out.append(" * FNaF3Scares.h — GENERATED data, do not edit by hand.")
out.append(" * The two FNAF3 death animations (the dump's \"jumpscare\" and")
out.append(" * \"jumpscare 2\"): 41 cells each at speed 60 -> 36 fps. Every cell")
out.append(" * carries its own size and hotspot, so the draw origin is stored")
out.append(" * per cell (the create position minus the cell's hotspot).")
out.append(" * VS2010/C++03.")
out.append(" */")
out.append("#ifndef FNAF_FNAF3SCARES_H")
out.append("#define FNAF_FNAF3SCARES_H")
out.append("")
out.append("namespace fnaf {")
out.append("")
out.append("struct FNaF3ScareCell {")
out.append("    unsigned short img; short left, top;")
out.append("    unsigned short w, h;")
out.append("};")
out.append("")

for prefix, handle, ix, iy in SCARES:
    cells = cells_of(handle)
    out.append(f"static const FNaF3ScareCell {prefix}[{len(cells)}] = {{")
    for img in cells:
        w, h, hx, hy = hot.get(img, (0, 0, 0, 0))
        out.append("    { %5d, %6d, %6d, %5d, %5d }," % (img, ix - hx, iy - hy, w, h))
    out.append("};")
    out.append(f"static const int {prefix}Count = {len(cells)};")
    out.append("")

out.append("} // namespace fnaf")
out.append("")
out.append("#endif // FNAF_FNAF3SCARES_H")
out.append("")

open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(out))
print(f"wrote {OUT}")
for prefix, handle, _ix, _iy in SCARES:
    print(f"   {prefix}: {len(cells_of(handle))} cells")
