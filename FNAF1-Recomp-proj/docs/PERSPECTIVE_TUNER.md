# PERSPECTIVE TUNER (v2.7.11-persptune)

Live bulge/concavity tuning for the clean-room Perspective port. The three
serialized PANORAMA constants of the original `Perspective.mfx` object are
run-time knobs now, so the bend can be matched **by eye** on real hardware
instead of by recompiling.

Defaults are the exact serialized EDATA values (`Zoom=300`, pivot `Y=355`,
arc `pi=3.1415`) -- with nobody touching the tuner the game renders
**bit-identical to v2.7.10-camfix**.

## Controls

| Input        | Action                                          |
|--------------|-------------------------------------------------|
| L3 + R3      | open / close the tuner (works in every state)   |
| DPad Left/Right | select knob: ZOOM -> CENTER_Y -> ARC         |
| DPad Up/Down | adjust the selected knob (+/-)                  |
| A (hold)     | fast step (x5)                                  |
| Y            | reset the selected knob to the original value   |

While the tuner is open during a night, gameplay input is swallowed (doors,
lights, camera nav are dead until you close it). The HUD in the top-left
corner shows all three values, the defaults, and the selected row.

## Knobs (what each one does to the picture)

The bend maps every ~8-px column of the scene to a height

    h(x) = 754 + sin(step(x)) * ZOOM - ZOOM

so the **center column is always full height** and the **edges carry the
effect**:

* **ZOOM** (default `300`)
  * `+` — bulge / fish-eye: edge columns are `754 - ZOOM` px tall, the
    scene bulges toward you (300 = edges squeezed 754 -> 454 px).
  * `0` — dead flat pan (no bend at all; useful as a baseline).
  * `-` — pincushion / concave: edge columns stretch OUT beyond 754 px.
  * Range -500..+500, step 10, fast 50.
* **CENTER_Y** (default `355`, the object center `-22 + 754/2`)
  * Vertical pivot of the bend. Lower = the curve pinches higher on
    screen, higher = pinches lower. Range 100..640, step 2, fast 10.
* **ARC** (default `3.1415` — Andos' truncated pi)
  * Span of the sine across the 1324-px object. `3.1415` = the original:
    edge columns reach `sin = 0` exactly. Bigger values push the sine
    negative at the edges -> hard fisheye with collapsed corners;
    smaller = gentler, flatter falloff. Range 1.0..6.0, step 0.05, fast 0.25.

## Baking a tuned look

Closing the tuner prints the current values to the log and to the on-screen
debug console:

    PERSP FINAL: ZOOM=245.0 CENTER_Y=349.0 ARC=3.1415

Paste them over the three initializers in `src/GameRender.cpp` (right below
`kBentVertCap`) and rebuild:

    static f32 g_perspZoom    = PERSP_ZOOM;      // 300.0
    static f32 g_perspCenterY = PERSP_CENTER_Y;  // 355.0
    static f32 g_perspArc     = PERSP_PI;        // 3.1415

i.e. for the example above: `g_perspZoom = 245.0f; g_perspCenterY = 349.0f;`.
The values also survive state changes (menu -> night -> power out) but not
power cycles -- bake anything you want to keep.
