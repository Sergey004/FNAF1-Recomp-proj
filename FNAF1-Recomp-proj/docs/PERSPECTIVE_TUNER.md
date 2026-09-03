# PERSPECTIVE TUNER (v2.8)

Live bulge/concavity tuning for the clean-room Panorama **shader** port.
The three serialized PANORAMA parameters of the original `Perspective.mfx`
object are run-time knobs now, so the bend can be matched **by eye** on real
hardware instead of by recompiling.

Defaults are the exact serialized values (`Zoom=300`, pivot `Y=355`, and the
`RPanorama.fx` parabola literal `4.0`) -- with nobody touching the tuner the
game renders the original look.

## Controls

| Input        | Action                                          |
|--------------|-------------------------------------------------|
| L3 + R3      | open / close the tuner (works in every state)   |
| DPad Left/Right | select knob: ZOOM -> CENTER_Y -> CURVE       |
| DPad Up/Down | adjust the selected knob (+/-)                  |
| A (hold)     | fast step (x5)                                  |
| Y            | reset the selected knob to the original value   |

While the tuner is open during a night, gameplay input is swallowed (doors,
lights, camera nav are dead until you close it). The HUD in the top-left
corner shows all three values, the defaults, and the selected row.

## Knobs (what each one does to the picture)

The shader re-samples each column with a parabola:

    fB     = 1 - ZOOM / 754
    fC(u)  = max(0.02, 1 + (fB - 1) * CURVE * (u - 0.5)^2)
    src.y  = (v - pivot) * fC + pivot

so the **center column is 1:1** and the **edges carry the effect** (the
central band of the source is stretched to the full window height there).

* **ZOOM** (default `300`)
  * `+` — bulge / fish-eye: edges read a narrower source band (300 = edges
    squeeze to ~62% of the source height).
  * `0` — dead flat pan (no bend at all; useful as a baseline).
  * `-` — pincushion / concave: edges read a WIDER band (stretch out).
  * Range -500..+500, step 10, fast 50.
* **CENTER_Y** (default `355`, the object center `-22 + 754/2`)
  * Vertical pivot of the bend. Lower = the curve pinches higher on
    screen, higher = pinches lower. Range 100..640, step 2, fast 10.
* **CURVE** (default `4.0` — the parabola coefficient that replaces the
  `RPanorama.fx` literal `4.0`)
  * Shape of the falloff. `4.0` = the original. `0` = flat (fC = 1
    everywhere). `>4` = sharper edge squeeze. Range 0..8, step 0.25, fast 1.

## Baking a tuned look

Closing the tuner prints the current values to the log and to the on-screen
debug console:

    PERSP FINAL: ZOOM=245.0 CENTER_Y=349.0 CURVE=4.00

Paste them over the three initializers in `src/GameRender.cpp` and rebuild:

    static f32 g_perspZoom    = PERSP_ZOOM;      // 300.0
    static f32 g_perspCenterY = PERSP_CENTER_Y;  // 355.0
    static f32 g_perspCurve   = PERSP_CURVE;     // 4.0

i.e. for the example above: `g_perspZoom = 245.0f; g_perspCenterY = 349.0f;`.
The values also survive state changes (menu -> night -> power out) but not
power cycles -- bake anything you want to keep.