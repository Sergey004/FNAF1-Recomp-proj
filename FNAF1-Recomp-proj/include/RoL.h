/**
 * Five Nights at Freddy's 1 — Recompilation
 * RoL.h: SMC Ring of Light — quadrant LEDs + center power LED (v2.97)
 *
 * Talks to the Southbridge Micro Controller (SMC) through
 * HalSendSMCMessage (xboxkrnl.lib), messages per free60.org/SMC:
 *   0x99 0x01 <leds> — quadrant states: high nibble = GREEN bits,
 *                      low nibble = RED bits; quadrants TL,TR,BL,BR
 *                      (console lying flat) map to bits 4-7 / 0-3.
 *                      ORANGE = both bits. OFF = none.
 *   0x8c <cmd> <anim> — center power LED (ON/DEFAULT/OFF/BLINK).
 *
 * SMC traffic is a slow SMBus transaction, so the module caches the
 * desired state and only sends on a CHANGE (min ~100 ms apart);
 * call Tick(dt) once per frame to flush deferred sends. Sending the
 * same packed byte twice is skipped entirely — safe to call the
 * setters every tick (OnPowerUpdate fires 60x/s during gameplay).
 *
 * FNAF1 hooks (main.cpp): the quadrants mirror the power meter
 * ("Power left: NN%" — 4 green divisions draining to 1 red), the
 * whole ring goes RED for a jumpscare, the ring goes dark when the
 * power runs out, and returns to the dashboard default on 6 AM /
 * game over. The center power LED is left to the system.
 *
 * HARDWARE NOTE (v2.97, verified on consoles): the 0x99 quadrant
 * messages drive the Ring of Light on PHAT models (Falcon/Jasper)
 * only. On Slim (Trinity/Corona) the SMC generation differs and the
 * quadrants stay in the system state — the boot self-test is the
 * quick way to tell the two apart.
 */

#ifndef FNAF_ROL_H
#define FNAF_ROL_H

namespace rol {

// Quadrant LED color (free60 two-bit encoding, kept in the raw layout
// so the packing shift matches the Lightshow/SMC reference bit-for-bit)
enum QuadrantState {
    Q_OFF    = 0x00,
    Q_RED    = 0x08,
    Q_GREEN  = 0x80,
    Q_ORANGE = 0x88
};

// Quadrant order: 0=TopLeft, 1=TopRight, 2=BottomLeft, 3=BottomRight
// (console lying flat; on an upright console the ring reads rotated,
// the divisions stay a contiguous run either way).
void SetQuadrants(QuadrantState q0, QuadrantState q1,
                  QuadrantState q2, QuadrantState q3);
void SetSolid(QuadrantState color);   // all four divisions the same
void SetAllOff();                     // dark ring (dash re-lights Q1 green
                                      // when a controller is connected)
void RestoreDefault();                // dark ring — "not our business anymore"

// Center power LED (0x8c command byte). Untouched by the FNAF1 hooks.
static const unsigned char POWER_LED_ON     = 0x01;
static const unsigned char POWER_LED_DEFAULT= 0x02;
static const unsigned char POWER_LED_OFF    = 0x03;
static const unsigned char POWER_LED_BLINK  = 0x10;
void SetPowerLED(unsigned char command, bool animate);

// Per-frame flush of a change deferred by the send throttle (cheap no-op
// while the cached state is clean).
void Tick(float dt);

// v2.97 diagnostics. DebugSink fires on every ACTUAL SMC write with a
// short "RoL send 0x99 <byte>" line (main.cpp wires it to the on-screen
// debug console). RoLSelfTest cycles the ring GREEN -> ORANGE -> RED ->
// dark at boot using the exact Lightshow Sleep() pattern — the decisive
// check that HalSendSMCMessage reaches the SMC in this XEX at all.
typedef void (*DebugSink)(const char* msg);
void SetDebugSink(DebugSink fn);
void RoLSelfTest();

// v2.97: the guide-menu quit is handled by the app itself — the GUIDE
// button is a normal XInput edge here (v2.77), and its handler
// (ExitToDashboard) hands the ring back before XLaunchNewImage. No kernel
// notification API is involved (XNotifyGetLast is an xam.xex export,
// absent from xboxkrnl.lib).

} // namespace rol

#endif // FNAF_ROL_H
