#!/usr/bin/env python3
"""Generate the Sister Location room tables (the art of every room, exactly
as the dump's frame-start state draws it).

The ctfak "Frame Runtime Renderer" evaluates the frame's Start-of-Frame /
always event groups and then dumps the VISIBLE sprites per frame
(FrameRuntime/frame_N_<name>.sprites.txt): the layer, the objectInfo, the
object name, the image handle, the draw origin and the size.

From those lists this generator builds, per room:

  * the static sprites (the room art, the plates, the icons), and
  * the animated objects (the room's own cell sequences), with every cell's
    own size and hotspot - the cells are drawn at the instance point minus
    the cell's hotspot, exactly like the dump draws them.

The black plates (the "black" / "fade" overlays) are skipped: they are
visible in the dump only with an alpha coefficient of 255 (invisible).

Inputs:  JSON/application.json, Images/hotspots.txt,
         FrameRuntime/frame_N_<name>.sprites.txt
Output:  include/SLRooms.h

Usage: python3 tools/gen_sl_rooms.py "<dumps>/Sister Location" include/SLRooms.h
"""
import json
import os
import sys

DUMPS = sys.argv[1]
OUT = sys.argv[2]

data = json.load(open(os.path.join(DUMPS, "JSON", "application.json"), encoding="utf-8"))
objs = {o["handle"]: o for o in data["objects"]}
frames = list(data["frames"])

hot = {}
with open(os.path.join(DUMPS, "Images", "hotspots.txt"), encoding="utf-8") as f:
    for line in f:
        p = line.split()
        if len(p) == 5:
            hot[int(p[0])] = (int(p[1]), int(p[2]), int(p[3]), int(p[4]))

# the rooms the port renders (the dump's frame names)
ROOMS = [
    ("Elevator", "SLElev"),
    ("Vent crawl", "SLVent"),
    ("Main Hub", "SLHub"),
    ("Baby's Room", "SLBaby"),
    ("Ballora Gallery", "SLBallora"),
    ("Breaker Room", "SLBreaker"),
    ("Funtime Auditorium", "SLFuntime"),
]

# the black plates: they are drawn with an alpha coefficient of 255 at the
# frame start (invisible), so the port must not draw them
BLACK_PLATES = {292, 296, 297, 301, 307, 309, 688, 654, 2127, 42}
# the names that are event-gated overlays of the same kind
SKIP_NAMES = {"jumpscare", "jumpscare 2", "foxy scare", "Ennard_scare",
              "drops", "fade out", "fade in", "fade out 2", "black",
              "recharge", "blue flare"}


def read_sprites(index):
    """the visible-sprite list of a frame (the frame-start state)"""
    path = None
    fdir = os.path.join(DUMPS, "FrameRuntime")
    for name in os.listdir(fdir):
        if name.startswith("frame_%d_" % index) and name.endswith(".sprites.txt"):
            path = os.path.join(fdir, name)
            break
    if not path:
        return []
    rows = []
    with open(path, encoding="utf-8") as f:
        next(f, None)
        for line in f:
            p = line.rstrip("\n").split("\t")
            if len(p) < 8:
                continue
            rows.append({
                "layer": int(p[0]),
                "objInfo": int(p[1]),
                "name": p[2],
                "img": int(p[3]),
                "left": int(p[4]),
                "top": int(p[5]),
                "w": int(p[6]),
                "h": int(p[7]),
                "anim": int(p[8]) if len(p) > 8 else 0,
                "direction": int(p[9]) if len(p) > 9 else 0,
            })
    return rows


def animations(obj):
    common = obj.get("common") or {}
    return common.get("animations") or {}


def instance_point(frame, index, seen=None):
    """the absolute instance point of a frame instance

    A child's X/Y are offsets from the PARENT OBJECT's instance point, so
    the absolute point is the parent's point plus the child's offset.
    """
    inst = frame["instances"][index]
    x, y = inst["x"], inst["y"]
    if inst.get("parentType") == 2 and inst.get("parentHandle"):
        for j, other in enumerate(frame["instances"]):
            if other["objectInfo"] == inst["parentHandle"]:
                px, py = instance_point(frame, j)
                return px + x, py + y
    return x, y


def extra_instances(frame, obj_info, row, hot):
    """the other instances of an object (the elevator's shaft plate)"""
    out = []
    for i, inst in enumerate(frame["instances"]):
        if inst["objectInfo"] != obj_info:
            continue
        px, py = instance_point(frame, i)
        img = row["img"]
        w, h, hx, hy = hot.get(img, (row["w"], row["h"], 0, 0))
        left = px - hx
        top = py - hy
        # skip the instance the runtime state already reported (the event
        # groups may have moved it by a couple of pixels)
        if abs(left - row["left"]) <= 8 and abs(top - row["top"]) <= 8:
            continue
        out.append((img, left, top, w, h))
    return out


def find_sequence(obj, img, anim, direction):
    """the animation sequence whose first cell is the drawn image

    Returns (cells, speed) or None. The dump's frame-start state may have
    switched the object to another sequence (the hub's fans), so the search
    is by the drawn image first and by the sequence index second.
    """
    anims = animations(obj)
    # the exact (direction, sequence) the runtime used
    seqs = anims.get(str(direction)) or {}
    fr = seqs.get(str(anim))
    if fr and (fr.get("frames") or []) and fr["frames"][0] == img:
        return fr["frames"], fr.get("minSpeed") or 50
    # any sequence that starts with the drawn image
    for drn in sorted(anims.keys(), key=lambda k: int(k)):
        for _seq, fr in anims[drn].items():
            frames = fr.get("frames") or []
            if frames and frames[0] == img:
                return frames, fr.get("minSpeed") or 50
    # the default direction
    for drn in ("0",):
        seqs = anims.get(drn) or {}
        for _seq, fr in seqs.items():
            frames = fr.get("frames") or []
            if frames:
                return frames, fr.get("minSpeed") or 50
    return None


out = []
out.append("/**")
out.append(" * SLRooms.h - GENERATED data, do not edit by hand.")
out.append(" * The Sister Location rooms as the dump draws them: the sprites")
out.append(" * the frame-start state leaves visible and the animated objects")
out.append(" * with every cell's own size and hotspot. The coordinates are in")
out.append(" * the room's own space (the dump's world); the renderer scales")
out.append(" * the room to the 1280x720 window (the height-squash convention).")
out.append(" * VS2010/C++03.")
out.append(" */")
out.append("#ifndef FNAF_SLROOMS_H")
out.append("#define FNAF_SLROOMS_H")
out.append("")
out.append("namespace fnaf {")
out.append("")
out.append("struct SLRoomSprite {")
out.append("    unsigned short img; short x, y;")
out.append("    unsigned short w, h;")
out.append("};")
out.append("")
out.append("struct SLRoomAnimCell {")
out.append("    unsigned short img; short x, y;")
out.append("    unsigned short w, h;")
out.append("};")
out.append("")
out.append("struct SLRoomAnim {")
out.append("    const SLRoomAnimCell* cells; int count; float fps;")
out.append("};")
out.append("")

for frame_name, prefix in ROOMS:
    index = None
    for i, fr in enumerate(frames):
        if fr["name"] == frame_name:
            index = i
            break
    if index is None:
        print("!! frame not found: %s" % frame_name)
        continue
    frame = frames[index]
    rows = read_sprites(index)

    sprites = []
    anims = []
    # one row per object: the runtime state reports an object with several
    # instances once per instance, all at the same (collapsed) position
    seen_obj = set()
    for row in rows:
        if row["objInfo"] in seen_obj:
            continue
        seen_obj.add(row["objInfo"])
        obj = objs.get(row["objInfo"])
        if not obj:
            continue
        if obj.get("type") not in ("Active", "Backdrop"):
            continue
        name = row["name"]
        img = row["img"]
        if name in SKIP_NAMES or img in BLACK_PLATES:
            continue
        # img 300 is the dark plate: the rooms draw it as the "Active 3"
        # overlay (invisible at the frame start), the hub and Baby's room
        # draw it as the backdrop
        if img == 300 and not name.startswith("Backdrop"):
            continue
        if img < 0:
            continue
        # the room's own bounds: the HUD plates outside the room are clipped
        if row["left"] >= frame["width"] or row["top"] >= frame["height"]:
            continue
        if row["left"] + row["w"] <= 0 or row["top"] + row["h"] <= 0:
            continue

        seq = find_sequence(obj, img, row["anim"], row["direction"])
        if seq and len(seq[0]) > 1:
            cells, speed = seq
            # the instance point: the drawn cell's origin plus its hotspot
            hw, hh, hx, hy = hot.get(img, (row["w"], row["h"], 0, 0))
            px = row["left"] + hx
            py = row["top"] + hy
            table = []
            for c in cells:
                cw, ch, chx, chy = hot.get(c, (0, 0, 0, 0))
                table.append((c, px - chx, py - chy, cw, ch))
            anims.append((name, table, speed * 0.6))
        else:
            # an object with several instances (the elevator's shaft plate
            # is drawn twice: above and below the cabin) - the runtime
            # state keeps one instance, so the extra ones come from the
            # frame's own instance list
            extra = extra_instances(frame, row["objInfo"], row, hot)
            sprites.append((img, row["left"], row["top"], row["w"], row["h"]))
            for e in extra:
                if e[0] >= frame["width"] or e[1] >= frame["height"]:
                    continue
                if e[0] + e[2] <= 0 or e[1] + e[3] <= 0:
                    continue
                sprites.append(e)

    out.append("// frame %d: \"%s\" (%dx%d)" % (index, frame_name, frame["width"], frame["height"]))
    out.append("static const int %sRoomW = %d;" % (prefix, frame["width"]))
    out.append("static const int %sRoomH = %d;" % (prefix, frame["height"]))
    if sprites:
        out.append("static const SLRoomSprite %sSprites[] = {" % prefix)
        for s in sprites:
            out.append("    { %5d, %6d, %6d, %5d, %5d }," % s)
        out.append("};")
        out.append("static const int %sSpriteCount = %d;" % (prefix, len(sprites)))
    else:
        out.append("static const SLRoomSprite* %sSprites = 0;" % prefix)
        out.append("static const int %sSpriteCount = 0;" % prefix)
    for i, (name, table, fps) in enumerate(anims):
        out.append("static const SLRoomAnimCell %sAnim%d[] = {" % (prefix, i))
        for c in table:
            out.append("    { %5d, %6d, %6d, %5d, %5d }," % c)
        out.append("};")
    if anims:
        out.append("static const SLRoomAnim %sAnims[] = {" % prefix)
        for i, (name, table, fps) in enumerate(anims):
            out.append("    { %sAnim%d, %d, %.1ff }," % (prefix, i, len(table), fps))
        out.append("};")
        out.append("static const int %sAnimCount = %d;" % (prefix, len(anims)))
        # the index of every animated object by name - the gameplay moves
        # some of them itself (the Ballora's dancer)
        used = {}
        for i, (name, table, fps) in enumerate(anims):
            ident = "".join(c if c.isalnum() else "_" for c in name)
            if not ident or ident[0].isdigit():
                ident = "Obj" + ident
            ident = ident[0].upper() + ident[1:]
            used[ident] = used.get(ident, 0) + 1
            if used[ident] > 1:
                ident = "%s%d" % (ident, used[ident])
            out.append("static const int %sAnim%s = %d;" % (prefix, ident, i))
    else:
        out.append("static const SLRoomAnim* %sAnims = 0;" % prefix)
        out.append("static const int %sAnimCount = 0;" % prefix)
    out.append("")

out.append("} // namespace fnaf")
out.append("")
out.append("#endif // FNAF_SLROOMS_H")
out.append("")

open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(out))
print("wrote %s" % OUT)
for frame_name, prefix in ROOMS:
    index = [i for i, fr in enumerate(frames) if fr["name"] == frame_name]
    if not index:
        continue
    rows = read_sprites(index[0])
    print("   %-20s %d visible rows" % (frame_name, len(rows)))
