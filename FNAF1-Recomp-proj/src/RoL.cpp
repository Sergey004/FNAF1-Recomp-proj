/**
 * Five Nights at Freddy's 1 — Recompilation
 * RoL.cpp: SMC Ring of Light — thin adapter over the VENDORED Lightshow
 * SMC.h (v2.97)
 *
 * v2.97 redesign (user call): take Lightshow's SMC code WHOLESALE. The
 * LED message construction now runs through the VERBATIM Lightshow
 * functions (PrepareBuffers/SetPowerLED/SetSolidRing/SetLEDs, copied 1:1
 * from Lightshow-boot.cpp) operating on the same New_SMC_State[4]
 * globals. RoL only adds the game-facing layer on top:
 *   - a desired-state cache + dedup byte (repeat calls never touch the
 *     bus — OnPowerUpdate fires 60x/s during gameplay),
 *   - a ~100 ms SMBus guard window; a change landing inside it is
 *     parked until the next Tick() flush,
 *   - a debug sink — every ACTUAL SMC write is reported (wired to the
 *     on-screen debug console by main.cpp),
 *   - a BOOT SELF-TEST cycling green/orange/red with Sleep() — the exact
 *     Lightshow execution pattern, proving the SMC path in this XEX
 *     independent of any game logic.
 */

#include <xtl.h>
#include <string.h>
#include "RoL.h"
#include "third_party/SMC.h"   // v2.97: vendored verbatim from Lightshow (CC0)
#include "XdkCompat.h"         // Snprintf

// ============================================================
//  VERBATIM Lightshow implementation (Lightshow-boot.cpp)
// ============================================================

LEDState New_SMC_State[4];
unsigned char Power_LED_State = POWER_LED_ON;

void PrepareBuffers() {
	ZeroMemory( m_SMCMessage, sizeof(m_SMCMessage) );
	ZeroMemory( m_SMCReturn, sizeof(m_SMCReturn) );
}

void SetPowerLED(unsigned char command, bool animate) {
    PrepareBuffers();
    m_SMCMessage[0] = 0x8c;
    m_SMCMessage[1] = command;
    m_SMCMessage[2] = (animate ? 0x01 : 0x00);
	HalSendSMCMessage(m_SMCMessage, NULL);
}

void SetSolidRing(LEDState color) {
	New_SMC_State[0] = color;
	New_SMC_State[1] = color;
	New_SMC_State[2] = color;
	New_SMC_State[3] = color;
	SetLEDs();
}

void SetLEDs() {
	LEDState sTopLeft = New_SMC_State[0];
	LEDState sTopRight = New_SMC_State[1];
	LEDState sBottomLeft = New_SMC_State[2];
	LEDState sBottomRight = New_SMC_State[3];
	PrepareBuffers();
    m_SMCMessage[0] = 0x99;
    m_SMCMessage[1] = 0x01;
	m_SMCMessage[2] = ((unsigned char)(sTopLeft>>3) | (unsigned char)(sTopRight>>2) | (unsigned char)(sBottomLeft>>1) | (unsigned char)(sBottomRight));
	HalSendSMCMessage(m_SMCMessage, NULL);
}

// ============================================================
//  RoL game-facing adapter
// ============================================================

namespace rol {

static const float MIN_SEND_INTERVAL = 0.1f;   // s between SMC writes

static unsigned char s_desired[4] = { Q_OFF, Q_OFF, Q_OFF, Q_OFF };
static unsigned char s_lastSent   = 0xFF;      // packed byte unknown at boot
static float         s_sinceSend  = MIN_SEND_INTERVAL;  // first send is free
static bool          s_deferred   = false;

// v2.97 diagnostics: reported on every ACTUAL SMC write (may be NULL)
static DebugSink s_sink = 0;

static unsigned char Packed(const unsigned char* q) {
    // Lightshow/tmbinc packing: green nibble = TL,TR,BL,BR (bits 4-7),
    // red nibble = TL,TR,BL,BR (bits 0-3).
    return (unsigned char)((q[0] >> 3) | (q[1] >> 2) | (q[2] >> 1) | q[3]);
}

static void SendDebug(unsigned char byte) {
    if (!s_sink) return;
    char buf[32];
    fnaf::Snprintf(buf, sizeof(buf), "RoL send 0x99 %02X", byte);
    s_sink(buf);
}

static void SendQuadrantMessage() {
    // Lightshow-verbatim path: globals -> SetLEDs() -> HalSendSMCMessage
    New_SMC_State[0] = (LEDState)s_desired[0];
    New_SMC_State[1] = (LEDState)s_desired[1];
    New_SMC_State[2] = (LEDState)s_desired[2];
    New_SMC_State[3] = (LEDState)s_desired[3];
    SetLEDs();
    s_lastSent  = Packed(s_desired);
    s_sinceSend = 0.0f;
    s_deferred  = false;
    SendDebug(s_lastSent);
}

// Diff-then-send: only a REAL change reaches the SMC.
static void ApplyDesired() {
    const unsigned char packed = Packed(s_desired);
    if (packed == s_lastSent) return;            // no-op, bus untouched
    if (s_sinceSend < MIN_SEND_INTERVAL) { s_deferred = true; return; }
    SendQuadrantMessage();
}

void SetQuadrants(QuadrantState q0, QuadrantState q1,
                  QuadrantState q2, QuadrantState q3) {
    s_desired[0] = (unsigned char)q0;
    s_desired[1] = (unsigned char)q1;
    s_desired[2] = (unsigned char)q2;
    s_desired[3] = (unsigned char)q3;
    ApplyDesired();
}

void SetSolid(QuadrantState color) {
    SetQuadrants(color, color, color, color);
}

void SetAllOff() {
    SetSolid(Q_OFF);
}

void RestoreDefault() {
    SetAllOff();   // all-off: the system/dash re-lights Q1 green for P1
}

// NOTE: explicit global scope — the verbatim ::SetPowerLED would otherwise
// be hidden by this namespace member and the call would recurse.
void SetPowerLED(unsigned char command, bool animate) {
    ::SetPowerLED(command, animate);
}

void Tick(float dt) {
    s_sinceSend += dt;
    if (s_deferred && s_sinceSend >= MIN_SEND_INTERVAL) {
        SendQuadrantMessage();   // parked change from inside the guard window
    }
}

void SetDebugSink(DebugSink fn) {
    s_sink = fn;
}

void RoLSelfTest() {
    // v2.97 DECISIVE diagnostic: the exact Lightshow execution pattern.
    // The ring must visibly cycle GREEN -> ORANGE -> RED -> dark at boot.
    // If it does, HalSendSMCMessage works in this XEX and any missing
    // in-game effect is game-logic timing (the debug sink shows every
    // send). If it does NOT, the SMC path itself is dead in this XEX and
    // no game hook can fix it — compare the XEX against Lightshow's.
    SetSolidRing(GREEN);  Sleep(1000);
    SetSolidRing(ORANGE); Sleep(1000);
    SetSolidRing(RED);    Sleep(1000);
    SetSolidRing(OFF);    Sleep(150);
}

// v2.97 note: the guide-menu quit does NOT need kernel notifications.
// On this build the app sees the GUIDE button itself (XInput big-button
// edge, v2.77) and exits through ExitToDashboard(), which hands the ring
// back BEFORE XLaunchNewImage. XNotifyGetLast is an xam.xex export — NOT
// present in xboxkrnl.lib (LNK2019) — so a kernel-notification poll would
// need runtime resolution; the in-app handler covers the same flow.

} // namespace rol
