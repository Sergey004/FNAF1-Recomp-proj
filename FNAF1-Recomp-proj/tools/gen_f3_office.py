#!/usr/bin/env python3
"""Generate the FNAF3 office sprite table (the phantom + dash animations).

The dump's office frame (3 "Frame 1") carries four sprites whose motion
lives in their ANIMATION CELLS, not in a position action:

  "run past"  (objInfo 126)  a child of "inwindow" - 16 cells, the Foxy
                               dashing past the office window
  "fwalk"     (objInfo 202)  a child of "inwindow" - 14 cells (the idle
                               walk) + 11 cells (the scare)
  "chica 2"   (objInfo 212)  a child of "asd" - 1 idle cell + 15 scare cells
  "foxy"      (objInfo 218)  a child of "asd" - 1 idle cell + 13 scare cells

A child's position is relative to its parent's INSTANCE point (the
hotspot point), so the draw origin of every cell is

    the parent's absolute instance + the child's relative origin
    - the cell image's own hotspot

Inputs: JSON/application.json (the animations) + Images/hotspots.txt (the
per-image size and hotspot).

Usage: python3 tools/gen_f3_office.py "<dumps>/Five Nights at Freddys 3" include/FNaF3Office.h
"""
import json
import os
import sys

DUMPS = sys.argv[1]
OUT = sys.argv[2]

data = json.load(open(os.path.join(DUMPS, "JSON", "application.json"), encoding="utf-8"))
objs = {o["handle"]: o for o in data["objects"]}
frame = [x for x in data["frames"] if x["name"] == "Frame 1"][0]
inst = {i["objectInfo"]: i for i in frame["instances"]}

hot = {}
with open(os.path.join(DUMPS, "Images", "hotspots.txt"), encoding="utf-8") as f:
    for line in f:
        p = line.split()
        if len(p) == 5:
            hot[int(p[0])] = (int(p[1]), int(p[2]), int(p[3]), int(p[4]))


def instance_point(oi):
    """the absolute instance point (a child's is relative to its parent)

    The parent reference names the PARENT OBJECT (its handle = the
    objectInfo), not the parent instance: "run past" is a child of
    "inwindow" (the office window) - the alternative reading would make it
    a child of the "cam 13 text" label, which makes no sense.
    """
    i = inst[oi]
    x, y = i["x"], i["y"]
    if i.get("parentType") == 2 and i.get("parentHandle"):
        for j in frame["instances"]:
            if j["objectInfo"] == i["parentHandle"]:
                px, py = instance_point(j["objectInfo"])
                x, y = px + x, py + y
                break
    return x, y


# name -> (objInfo, {animation direction: (fps, cells)})
SPRITES = [
    ("RunPast", 126, [0]),
    ("FWalk", 202, [0, 12]),
    ("Chica2", 212, [0, 12]),
    ("Foxy", 218, [0, 12]),
]

out = []
out.append("/**")
out.append(" * FNaF3Office.h — GENERATED data, do not edit by hand.")
out.append(" * The four office sprites whose motion is baked into their animation")
out.append(" * cells: the run-past dash, the Golden Freddy walk and the two")
out.append(" * phantom faces. Every cell carries its own size and hotspot, so")
out.append(" * the draw origin is stored per cell (the parent's instance point")
out.append(" * plus the child's offset, minus the cell's hotspot).")
out.append(" * VS2010/C++03.")
out.append(" */")
out.append("#ifndef FNAF_FNAF3OFFICE_H")
out.append("#define FNAF_FNAF3OFFICE_H")
out.append("")
out.append("namespace fnaf {")
out.append("")
out.append("struct FNaF3OfficeCell {")
out.append("    unsigned short img; short left, top;")
out.append("    unsigned short w, h;")
out.append("};")
out.append("")

for prefix, oi, dirs in SPRITES:
    o = objs[oi]
    common = o.get("common") or {}
    anims = common.get("animations") or {}
    px, py = instance_point(oi)
    for drn in dirs:
        seqs = anims.get(str(drn), {})
        if not seqs:
            continue
        cells = []
        speed = 50
        for _seq, fr in seqs.items():
            cells = fr.get("frames") or []
            speed = fr.get("minSpeed") or 50
            break
        if not cells:
            continue
        # the child's relative origin (the instance point minus the hotspot)
        relx = inst[oi]["x"]
        rely = inst[oi]["y"]
        out.append(f"static const FNaF3OfficeCell {prefix}A{drn}[{len(cells)}] = {{")
        for img in cells:
            w, h, hx, hy = hot.get(img, (0, 0, 0, 0))
            # the absolute instance point + the cell's own hotspot offset
            left = px + relx - hx
            top = py + rely - hy
            out.append("    { %5d, %6d, %6d, %5d, %5d }," % (img, left, top, w, h))
        out.append("};")
        # the animation speed: CTF runs at 60 Hz, so the fps = speed * 0.6
        fps = speed * 0.6
        out.append(f"static const int {prefix}A{drn}Count = {len(cells)};")
        out.append(f"static const float {prefix}A{drn}Fps = {fps:.1f}f;")
        out.append("")

out.append("} // namespace fnaf")
out.append("")
out.append("#endif // FNAF_FNA3OFFICE_H")
out.append("")

open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(out))
print(f"wrote {OUT}")
for prefix, oi, dirs in SPRITES:
    anims = objs[oi].get("common", {}).get("animations", {})
    for drn in dirs:
        seqs = anims.get(str(drn), {})
        n = 0
        for _seq, fr in seqs.items():
            n = len(fr.get("frames") or [])
            break
        print(f"   {prefix}A{drn}: {n} cells")
