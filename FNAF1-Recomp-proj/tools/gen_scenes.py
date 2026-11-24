#!/usr/bin/env python3
"""Generate a scene table for a Clickteam game from the ctfak dumps.

The port's per-game renderers are hand-written, but the STATIC art of every
screen is dump-derived: the FrameLayout files give each instance's image,
origin (left/top = the instance point minus the hotspot) and layer, and the
Images folder gives the natural size.

The FrameLayout dump alone is not enough to draw a frame correctly, so the
generator also reads the ctfak JSON export (JSON/application.json) when it
is there and applies the rules Fusion itself applies at frame start:

  * the draw order is the LAYER order (the layout dump lists the instances
    in object order, not in the order they are painted - drawing them in
    dump order puts the top layer under the bottom one);
  * an object with "VisibleAtStart = false" is not drawn;
  * a Counter object draws a NUMBER, not its image (the dump hands it a
    single digit glyph) - it is skipped here and drawn as text instead;
  * a blank image (the "no image" placeholder) is skipped;
  * a full-frame opaque BLACK image is the frame's fade-to-black overlay
    (FIS2's img 660, for one) - it carries no art and would black the
    screen out, so it is skipped as well.

Usage: python3 tools/gen_scenes.py "<dumps>/Furys Rage" "Furys Rage" include/FuryRageScenes.h FuryRage [frame-filter]

The optional frame filter is a comma separated list of frame indices (e.g.
"37,38,39,43"); without it every frame of the dump is emitted.
"""
import glob
import json
import os
import re
import sys
from PIL import Image

DUMPS = sys.argv[1]
GAME = sys.argv[2]
OUT = sys.argv[3]
PREFIX = sys.argv[4]

LAYOUT = os.path.join(DUMPS, "FrameLayout")
IMAGES = os.path.join(DUMPS, "Images")
JSON_PATH = os.path.join(DUMPS, "JSON", "application.json")

sizes = {}
blank = {}
black = {}


def size_of(img):
    if img in sizes:
        return sizes[img]
    p = os.path.join(IMAGES, f"{img}.png")
    try:
        sizes[img] = Image.open(p).size
    except Exception:
        sizes[img] = (0, 0)
    return sizes[img]


def is_blank(img):
    """the dump's "no image" placeholder: nothing but transparent pixels"""
    if img in blank:
        return blank[img]
    p = os.path.join(IMAGES, f"{img}.png")
    try:
        px = list(Image.open(p).convert("RGBA").getdata())
    except Exception:
        blank[img] = True
        return True
    lit = sum(1 for q in px if q[0] + q[1] + q[2] > 12)
    blank[img] = lit == 0
    return blank[img]


def is_black(img):
    """a fully opaque, fully black image"""
    if img in black:
        return black[img]
    p = os.path.join(IMAGES, f"{img}.png")
    try:
        px = list(Image.open(p).convert("RGBA").getdata())
    except Exception:
        black[img] = False
        return False
    black[img] = all(q[3] > 8 and q[0] + q[1] + q[2] <= 12 for q in px)
    return black[img]


# ---- the JSON export (optional): the object facts Fusion starts from ----
obj_info = {}
frame_wh = {}
if os.path.exists(JSON_PATH):
    data = json.load(open(JSON_PATH, encoding="utf-8"))
    for o in data.get("objects", []):
        common = o.get("common") or {}
        new_flags = common.get("newFlags") or {}
        obj_info[o["handle"]] = {
            "type": o.get("type", ""),
            "visible": new_flags.get("VisibleAtStart"),
            "created": not (common.get("flags") or {}).get("DoNotCreateAtStart", False),
        }
    for f in data.get("frames", []):
        frame_wh[f["name"]] = (f.get("width", 0), f.get("height", 0))
    print(f"json export: {len(obj_info)} objects, {len(frame_wh)} frames")


def visible_at_start(obj_info_index, name):
    """True when the instance must be painted at frame start"""
    o = obj_info.get(obj_info_index)
    if o is None:
        return True
    if o["type"] == "Counter":
        return False                       # a number, not art
    if o["visible"] is False:
        return False
    return True


frames = []
for f in sorted(glob.glob(os.path.join(LAYOUT, "*.txt"))):
    name = os.path.basename(f)[:-4]
    m = re.match(r"frame_(\d+)_(.*)", name)
    if not m:
        continue
    idx = int(m.group(1))
    rows = []
    for ln in open(f, encoding="utf-8", errors="ignore"):
        p = ln.split()
        if len(p) < 10 or not p[-8].endswith(".png"):
            continue
        # ... | img | file | x | y | hotX | hotY | left | top | layer
        obj = int(p[0])
        img = int(p[-9])
        left = int(p[-3])
        top = int(p[-2])
        layer = int(p[-1])
        w, h = size_of(img)
        if not visible_at_start(obj, name):
            continue
        if w <= 0 or h <= 0:
            continue
        if is_blank(img):
            continue
        # a full-frame opaque black image is the frame's fade overlay
        fw, fh = frame_wh.get(name, (0, 0))
        if fw and fh and w == fw and h == fh and is_black(img):
            continue
        rows.append((img, left, top, w, h, layer, " ".join(p[1:-9])))
    # Fusion paints by layer (the layout dump lists the instances in object
    # order - the top layer would end up under the bottom one)
    rows.sort(key=lambda r: r[5])
    frames.append((idx, m.group(2), rows))
if len(sys.argv) > 5:
    wanted = set(int(x) for x in sys.argv[5].split(",") if x.strip())
    frames = [f for f in frames if f[0] in wanted]
frames.sort()

out = []
out.append("/**")
out.append(f" * {os.path.basename(OUT)} — GENERATED data, do not edit by hand.")
out.append(f" * Source: ctfak dump of {GAME!r} (the FrameLayout files of every")
out.append(" * frame). Each entry: image id, draw origin (left/top = the")
out.append(" * instance point minus the hotspot), natural size and layer.")
out.append(" * VS2010/C++03.")
out.append(" */")
guard = "FNAF_GENERATED_SCENES_" + re.sub(r"[^A-Za-z0-9]", "_", os.path.basename(OUT)).upper()
out.append("#ifndef " + guard)
out.append("#define " + guard)
out.append("")
out.append("namespace fnaf {")
out.append("")
out.append(f"struct {PREFIX}SceneInst {{")
out.append("    unsigned short img; short left, top;")
out.append("    unsigned short w, h; unsigned char layer;")
out.append("};")
out.append("")
out.append(f"static const {PREFIX}SceneInst {PREFIX}Scenes[] = {{")

frame_rows = []
for idx, fname, rows in frames:
    if not rows:
        continue
    for img, left, top, w, h, layer, _name in rows:
        out.append(f"    {{ {img:5d}, {left:6d}, {top:6d}, {w:5d}, {h:5d}, {layer} }},")
    frame_rows.append((idx, fname, len(rows)))
out.append("};")
out.append("")
out.append(f"struct {PREFIX}SceneFrame {{ int index; const char* name; int first; int count; }};")
out.append(f"static const {PREFIX}SceneFrame {PREFIX}SceneFrames[] = {{")
pos = 0
for idx, fname, count in frame_rows:
    out.append(f'    {{ {idx}, "{fname}", {pos}, {count} }},')
    pos += count
out.append("};")
out.append("")
out.append(f"static const int {PREFIX}SceneFrameCount = (int)(sizeof({PREFIX}SceneFrames)/sizeof({PREFIX}SceneFrames[0]));")
out.append("")
out.append("} // namespace fnaf")
out.append("")
out.append("#endif // " + guard)
out.append("")

open(OUT, "w", encoding="utf-8", newline="\n").write("\n".join(out))
print(f"wrote {OUT}: {len(frame_rows)} frames, {pos} instances")
for idx, fname, count in frame_rows:
    print(f"   frame {idx:2d} {fname!r}: {count}")
