/**
 * SLGame.cpp: v2.65 — Sister Location wave 1 (see SLGame.h).
 * Dump authority: Dumps/Sister Location/Events/*.txt + docs/SL_MECHANICS.md
 * (the pinned digest). Timers are RAW MILLISECONDS. The room frames are
 * storyboard slots directly (SL prints its jump names correctly, unlike
 * FNAF1/3/4 — verified); the global `go to` counter arms travel and the
 * frame-4 "load" router consumes it every 200 ms.
 * Deviations are labeled inline.
 */

#include "SLGame.h"
#include "XdkCompat.h"   // Snprintf
#include <stdlib.h>

namespace fnaf {

// ------------------------------------------------------------
// construction / helpers
// ------------------------------------------------------------

SLGame::SLGame() {
    ResetToTitle();
}

void SLGame::ResetToTitle() {
    m_screen = SCR_WARNING;
    m_night = 1;
    m_optionSelected = 0;
    m_started = false;
    m_dieRoute = 1;
    m_goTo = 0;
    m_scriptEvent = 0;
    m_print = 0;
    m_loadT = 0.0f;
    m_pan = 0.0f;
    m_panTarget = 0.0f;
    m_crawl = 0;
    m_wHeldOnce = false;
    m_wArmT = 0.0f;
    m_quick = 0;
    m_progress = 0;
    m_notch = 0;
    m_ventGoing = 0;
    m_stepSndT = 0.0f;
    m_distance = 0;
    m_moving = false;
    m_contWalkT = 0.0f;
    m_leftPan = 0;
    m_dancer = false;
    m_panT = 0.0f;
    m_moveT = 0.0f;
    m_dirSide = 0;
    m_flashCharge = 50.0f;
    m_foxyDist = 0;
    m_foxyApproach = false;
    m_foxyRollT = 0.0f;
    m_foxyPos = 0;
    m_foxyPosT = 0.0f;
    m_backwards = false;
    m_haveKeycard = false;
    m_elevatorState = 0;
    m_elevatorT = 0.0f;
    m_elevButtonT = 0.0f;
    m_scareT = 0.0f;
    m_scareImg = 0;
    m_cardT = 0.0f;
    m_time = 0.0f;
    m_winT = 0.0f;
    m_tvT = 0.0f;
    m_girlT = 0.0f;
    m_saveDirty = false;
    m_current = 1;
    m_intro = false;
    m_beat1 = false;
    m_beat3 = false;
    m_keycard = false;
    m_endsceneno = 0;
    m_star104 = false;
}

void SLGame::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void SLGame::SfxStop(const char* s) {
    if (audio.stop) audio.stop(s);
}
void SLGame::SfxPan(i32 ch, i32 pan) {
    if (audio.pan) audio.pan(ch, pan);
}

// ------------------------------------------------------------
// the go-to router (frame 4 "load"): arm a code -> fade -> SCR_LOAD eats
// it every 200 ms (five hits = the ~1 s trip)
// ------------------------------------------------------------

void SLGame::GoTo(i32 code) {
    m_goTo = code;
    ArmRouter();
}

void SLGame::ArmRouter() {
    m_loadT = 0.0f;
    m_screen = SCR_LOAD;    // the fade begins now
}

void SLGame::TickLoad(f32 dt) {
    m_loadT += dt;
    if (m_loadT >= 0.2f) {
        // the 200 ms consume (the print counter blocks travel — we always
        // have it released when the router opens)
        m_loadT = 0.0f;
        switch (m_goTo) {
            case 1:  m_screen = SCR_ELEVATOR;  break;   // the ride
            case 3:  m_screen = SCR_HUB;       break;   // Circus Control
            case 5:
            case 8:  m_screen = SCR_TOVENT;    break;   // the vent hop
            case 6:
            case 11: m_screen = SCR_BREAKER;   break;
            case 7:  m_screen = SCR_BALLORA;   break;
            case 10: m_screen = SCR_FUNTIME;   break;
            case 12: m_screen = SCR_BABY;      break;
            case 13: m_screen = SCR_SCOOPING;  break;
            case 30: m_screen = SCR_BATHROOM;  break;
            default: m_screen = SCR_LOAD; break;        // 2/4/9 hold (self)
        }
        m_goTo = 0;                                     // consumed
        if (m_screen != SCR_LOAD) m_cardT = 0.0f;
    }
}

// ------------------------------------------------------------
// tick
// ------------------------------------------------------------

void SLGame::Tick(f32 dt, const SLInputs& in) {
    m_time += dt;

    // the jumpscare override runs the scream then the game over
    if (m_scareT > 0.0f) {
        m_scareT += dt;
        if (m_scareT >= 1.4f) {
            m_scareT = 0.0f;
            m_screen = SCR_GAMEOVER;
            m_cardT = 0.0f;
        }
        return;
    }

    switch (m_screen) {
        case SCR_WARNING:   TickWarning(dt, in);   break;
        case SCR_TITLE:     TickTitle(in);        break;
        case SCR_ELEVATOR:  TickElevator(dt, in); break;
        case SCR_VENT:      TickVent(dt, in);     break;
        case SCR_LOAD:      TickLoad(dt);         break;
        case SCR_HUB:       TickHub(dt, in);      break;
        case SCR_BABY:      TickBaby(dt, in);     break;
        case SCR_TOVENT:    TickToVent(dt);       break;
        case SCR_BALLORA:   TickBallora(dt, in);  break;
        case SCR_BREAKER:   TickBreaker(dt, in);  break;
        case SCR_FUNTIME:   TickFuntime(dt, in);  break;
        case SCR_PS:
        case SCR_PS2:
            TickPS(dt, in);
            break;
        case SCR_WINNIGHT:  TickWinNight(dt);     break;
        case SCR_UNDERDESK: TickDesk(dt, in);     break;
        case SCR_EXTRAS:    TickExtras(dt, in);   break;
        case SCR_SCOOPING:
        case SCR_REDFADE:
        case SCR_BATHROOM:
        case SCR_CREDITS:
            TickChain(dt, in);                    // the night-4/5 end chain
            break;
        case SCR_DEATH:     TickDeath(dt, in);    break;
        case SCR_GAMEOVER:  TickGameOver(dt, in); break;
        case SCR_TVSHOW:    TickTvShow(dt);       break;
        case SCR_GIRLVOICE: TickGirlVoice(dt);    break;
        default:
            // the wave-2 screens (scooping chain, extras, custom night, the
            // 8-bit game, the keypad) hold with A -> title (labeled)
            m_cardT += dt;
            if (in.bPressed || m_cardT >= 12.0f) {
                m_cardT = 0.0f;
                m_screen = SCR_TITLE;
            }
            break;
    }
}

// ------------------------------------------------------------
// Warning (frame 0): the legal splash; INI load; Delete = the dump wipe
// ------------------------------------------------------------

void SLGame::TickWarning(f32 dt, const SLInputs& in) {
    m_cardT += dt;
    if (m_cardT >= 2.0f || in.aPressed || in.bPressed) {
        m_cardT = 0.0f;
        m_screen = SCR_TITLE;
        // the title theme (the dump: "Gradual Liquidation" loop)
        Sfx("snd_Gradual Liquidation", true, 2, 15);
    }
}

// ------------------------------------------------------------
// Title (frame 1): new game / continue / extras / custom (+custom wall)
// ------------------------------------------------------------

void SLGame::TickTitle(const SLInputs& in) {
    if (in.upPressed)
        m_optionSelected = (m_optionSelected + 3) % 4;
    if (in.downPressed)
        m_optionSelected = (m_optionSelected + 1) % 4;
    if (in.aPressed) {
        switch (m_optionSelected) {
            case 0:                                       // new game
                m_current = 1;
                m_night = 1;
                m_started = true;
                m_intro = false;
                m_keycard = false;
                m_endsceneno = 0;
                m_star104 = false;
                m_saveDirty = true;
                SfxStop("snd_Gradual Liquidation");
                NightStart();
                break;
            case 1:                                       // continue
                m_night = m_current < 1 ? 1 : m_current;
                m_started = true;
                SfxStop("snd_Gradual Liquidation");
                NightStart();
                break;
            case 2:
                m_screen = SCR_EXTRAS;                    // extras (wave 2 hold)
                m_cardT = 0.0f;
                break;
            case 3:
                if (m_beat3) {
                    m_screen = SCR_CUSTMENU;              // the custom night
                    m_cardT = 0.0f;
                }
                break;
        }
    }
}

// ------------------------------------------------------------
// the night presets (docs §6): the Script Event block per night + the
// start rooms
// ------------------------------------------------------------

void SLGame::NightStart() {
    // per-night Script Event presets (docs §5): night 1 starts the chain at
    // 0, night 2 at 50, night 3 at 100, night 4 resumes at 57, night 5 at 500
    static const i32 kPreset[6] = { 0, 0, 50, 100, 57, 500 };
    m_scriptEvent = kPreset[m_night < 0 ? 0 : (m_night > 5 ? 5 : m_night)];
    m_print = 0;
    m_progress = 0;
    m_crawl = 0;
    m_distance = 0;
    m_leftPan = 0;
    m_dancer = false;
    m_flashCharge = 50.0f;
    m_foxyDist = 0;
    m_notch = 0;
    m_ventGoing = 0;
    m_elevatorState = 0;
    m_elevatorT = 0.0f;
    m_elevButtonT = 0.0f;
    m_pan = 0.0f;
    m_panTarget = 0.0f;
    m_cardT = 0.0f;
    // every night ride starts in the elevator (the dump's per-night presets)
    m_screen = SCR_ELEVATOR;
    Sfx("snd_elevator ride loop", true, 1, 15);
}

// ------------------------------------------------------------
// Elevator (frame 2): the ride state machine. 40 s or A -> arrive;
// the doors open (state 4) -> the hub route (the day's room chain kicks
// off in wave 2; wave 1: the night-1 chain and a generic hub day)
// ------------------------------------------------------------

void SLGame::TickElevator(f32 dt, const SLInputs& in) {
    m_elevatorT += dt;
    // arrival after the ride (the dump: Active 3.alterable[18] 0..1500
    // ~40 s; the doors open at state 4 after the clank)
    if (m_elevatorState == 0 && (m_elevatorT >= 10.0f || in.aPressed)) {
        m_elevatorState = 1;
        Sfx("snd_elevator clank", false, 4, 100);
    }
    if (m_elevatorState == 1) {
        m_elevButtonT += dt;
        if (m_elevButtonT >= 2.0f || in.aPressed) {
            m_elevButtonT = 0.0f;
            m_elevatorState = 4;
            SfxStop("snd_elevator ride loop");
            // doors open -> the day plan: night 1 = Baby's Room (the
            // monologue), night 2 = Hub -> Ballora, night 3 = Hub ->
            // Breaker (wave 1 labeled; the spine is per Script Event)
            SfxStop("snd_elevator clank");
            Sfx("snd_hub hum", true, 2, 15);
            GoTo(3);
        }
    }
}

// ------------------------------------------------------------
// Main Hub (frame 5): the Circus Control — vents with availability flags
// + the shock buttons
// ------------------------------------------------------------

void SLGame::TickHub(f32 dt, const SLInputs& in) {
    (void)dt;
    // the day plan (wave 1, labeled): per night the hub forwards to that
    // night's room chain. Night 1 -> Baby's Room, night 2 -> Ballora via
    // the left vent, night 3 -> Breaker via the right vent, nights 4-5 ->
    // the P&S / scooping holds.
    if (m_cardT > 6.0f || in.aPressed) {
        switch (m_night) {
            case 1: GoTo(12); break;                    // Baby's Room
            case 2:
                m_ventGoing = 2;                        // the left vent flags
                GoTo(5);                                // -> the vent hop
                break;
            case 3:
                m_ventGoing = 3;
                GoTo(8);
                break;
            case 4:
                m_ventGoing = 2;
                GoTo(11);                               // -> Breaker (P&S chain)
                break;
            default:
                m_ventGoing = 3;
                GoTo(10);                               // -> Funtime
                break;
        }
    } else if (in.rightPressed) {
        Sfx("snd_denied", false, 6, 60);                // the denied blip
    }
    m_cardT += dt;
}

// ------------------------------------------------------------
// the vent hop (frame 7 "to vent"): 100 ms black -> Elevator
// (or wherever the armed code route lands — the dump's is always elevator)
// ------------------------------------------------------------

void SLGame::TickToVent(f32 dt) {
    m_cardT += dt;
    if (m_cardT >= 0.1f) {
        m_cardT = 0.0f;
        // nights 2/4 the vent leads to Ballora/Breaker, night 3 to the hub
        if (m_night == 2)      m_screen = SCR_VENT;
        else if (m_night == 4) m_screen = SCR_VENT;
        else if (m_night >= 5) m_screen = SCR_VENT;
        else                   m_screen = SCR_ELEVATOR;
    }
}

// ------------------------------------------------------------
// Vent crawl (frame 3): hold W (latch 15, Shift = loud), 100 held ticks =
// one notch, 10 notches = the vent traversed -> the vent-going route
// ------------------------------------------------------------

void SLGame::TickVent(f32 dt, const SLInputs& in) {
    if (in.wHeld) {
        m_crawl = 15;                                   // the latch
        m_notch += dt >= 1.0f ? 0 : 0;                  // (notch below)
    } else if (m_crawl > 0) {
        m_crawl -= 1;
    }
    const bool loud = (m_crawl > 0 && in.shiftHeld);
    const bool moving = (m_crawl > 0);
    if (moving) {
        // the duct sounds (the dump: fast ch2 / slow ch3 by loud)
        m_stepSndT += dt;
        if (m_stepSndT >= 0.5f) {
            m_stepSndT = 0.0f;
            Sfx(loud ? "snd_metal_duct_fast" : "snd_metal_duct_slow",
                false, loud ? 2 : 3, loud ? 100 : 50);
        }
        m_wArmT += dt;
        if (m_wArmT >= 0.1f) {                          // 100 ms ticks
            m_wArmT -= 0.1f;
            m_notch += 1;                               // held tick count
        }
    }
    if (m_notch >= 10 && m_ventGoing > 0) {
        // 10 notches = traversed -> the armed route (2/3/4 hold/hub/hold)
        m_notch = 0;
        m_ventGoing = 0;
        if (m_night == 2)      GoTo(7);                 // -> Ballora
        else if (m_night == 4) GoTo(11);                // -> Breaker
        else if (m_night >= 5) GoTo(13);                // -> scooping chain
        else                   GoTo(3);
    }
}

// ------------------------------------------------------------
// Ballora Gallery (frame 8) — "the dance" (docs §4): W fills progress,
// distance = how much she heard you (decay when standing), music pans by
// side, death at > 600
// ------------------------------------------------------------

void SLGame::TickBallora(f32 dt, const SLInputs& in) {
    // the arm latch (G2): W held once after 2 s arms the crawl
    if (!m_wHeldOnce) {
        m_wArmT += dt;
        if (m_wArmT >= 2.0f && in.wHeld) { m_wHeldOnce = true; m_crawl = 15; }
    } else if (in.wHeld) {
        m_crawl = 15;
    } else if (m_crawl > 0) {
        m_crawl -= 1;
    }

    // the walk fills progress ~1/tick (Shift 2/tick) (G41)
    if (m_crawl > 0 && in.wHeld) {
        m_quick += in.shiftHeld ? 2 : 1;
        if (m_quick >= 5) { m_quick = 0; m_progress += 1; }
        // the patter sounds by speed (G84/85)
        m_stepSndT += dt;
        if (m_stepSndT >= 0.4f) {
            m_stepSndT = 0.0f;
            Sfx("snd_patter", false, in.shiftHeld ? 2 : 3,
                in.shiftHeld ? 100 : 50);
        }
    } else {
        m_contWalkT = 0.0f;
        // standing still: the hearing decays (−1 per 30 ms)
        m_moveT += dt;
        if (m_moveT >= 0.03f) {
            m_moveT = 0.0f;
            if (m_distance > 0) m_distance -= 1;
        }
    }

    // she is created at progress 400 ("the dance" starts)
    if (!m_dancer && m_progress >= 400) {
        m_dancer = true;
        Sfx("snd_ballora box", true, 1, 100);           // her music
    }
    if (m_dancer) {
        // the music pans by side (-100..100 ramp, 1 per 20 ms)
        m_panT += dt;
        if (m_panT >= 0.02f) {
            m_panT -= 0.02f;
            if (m_dirSide == 0) { if (m_leftPan > -100) m_leftPan -= 1; }
            else               { if (m_leftPan <  100) m_leftPan += 1; }
            SfxPan(1, m_leftPan);
        }
        // her approach rolls: walking builds it, standing decays it; 5 s of
        // continuous walking puts her into the active approach (moving)
        if (in.wHeld) {
            m_contWalkT += dt;
            m_distance += 1;
            if (m_contWalkT >= 5.0f) {
                m_moving = true;                        // moving toward 2
                m_distance += 1;
            }
        } else {
            m_contWalkT = 0.0f;
        }
        // the side changes on her move rolls
        m_moveT += dt;
        if (m_moveT >= 3.0f) {
            m_moveT = 0.0f;
            m_dirSide = rand() % 2;
        }
        if (m_distance > 600) {                         // the kill
            m_scareT = 0.001f;
            m_scareImg = 1;                             // Ballora scare
            SfxStop("snd_ballora box");
            Sfx("snd_scream op1-1", false, 4, 100);
        }
    } else if (m_progress >= 300 && m_distance < 200) {
        // the "quickly!" hint — the renderer shows it
    }

    // the exits (G41 the dump edges): 850 fwd -> the gallery reached;
    // 650 back -> Baby's Room
    if (m_progress >= 850) {
        SfxStop("snd_ballora box");
        GoTo(11);                                       // -> the Breaker end
    }
}

// ------------------------------------------------------------
// Funtime Auditorium (frame 10): Space = flash (ATTRACTS Foxy: +50
// distance, 2 s refill), distance > 500 flashing / > 600 walking = caught;
// the backwards trip ends the night (night 3 -> 4 transition per dump)
// ------------------------------------------------------------

void SLGame::TickFuntime(f32 dt, const SLInputs& in) {
    // the flash recharge (2 s full, +1 per 40 ms)
    if (m_flashCharge < 50.0f) {
        m_flashCharge += dt / 0.04f;
        if (m_flashCharge > 50.0f) m_flashCharge = 50.0f;
    }
    // the walk (W fills progress, Shift 2/tick)
    if (in.wHeld) {
        m_progress += in.shiftHeld ? 2 : 1;
        m_foxyDist += in.shiftHeld ? 2 : 1;
    } else if (m_foxyDist > 0) {
        m_moveT += dt;
        if (m_moveT >= 0.03f) {
            m_moveT = 0.0f;
            m_foxyDist -= 1;                          // standing decays him
        }
    }
    // Space = the flash beacon
    if (in.flasherPressed && m_flashCharge >= 50.0f) {
        m_flashCharge = 0.0f;
        m_foxyDist += 50;                               // it ATTRACTS him
        Sfx("snd_flash", false, 4, 100);
    }
    // Foxy's approach rolls (Random(2) every 3 s); approaching => built
    m_foxyRollT += dt;
    if (m_foxyRollT >= 3.0f) {
        m_foxyRollT = 0.0f;
        m_foxyApproach = (rand() % 2) == 0;
    }
    if (m_foxyApproach) m_foxyDist += 1;
    if (m_foxyDist > 600) m_foxyDist = 600;
    // the silhouettes reposition every 2 s
    m_foxyPosT += dt;
    if (m_foxyPosT >= 2.0f) {
        m_foxyPosT = 0.0f;
        m_foxyPos = rand() % 3;
    }
    // the got-you (dump): distance > 500 while flashing / > 600 walking
    if ((m_flashCharge < 50.0f && m_foxyDist > 500) || m_foxyDist > 600) {
        m_scareT = 0.001f;
        m_scareImg = 2;                                 // the Foxy scare
        Sfx("snd_scream op5-2", false, 5, 100);
    }
    // the exits: 2000 fwd reaches the end; the backwards run (B) ends the
    // night run (the always-fatal dump scare on the backward trip)
    if (m_progress >= 2000) {
        SfxStop("snd_Gradual Liquidation");
        GoTo(12);                                       // -> Baby's Room
    }
    if (in.bPressed) m_backwards = true;
}

// ------------------------------------------------------------
// Breaker Room (frame 9) — wave 1 skeleton: the task hold; walk beats to
// the win chain (labeled); night 4 after this room -> P&S (marked)
// ------------------------------------------------------------

void SLGame::TickBreaker(f32 dt, const SLInputs& in) {
    // v2.66: the breaker task — hold A to fill each of three panels (the
    // dump's 4-s fill gates); Freddie Funtime gets louder the longer it
    // runs (his counter +1 per noise roll) and kills at 7+. S exits at
    // Script Event 85. (Dump: frame 9 "Breaker Room"; kills -> game over.)
    if (m_cardT == 0.0f) {
        Sfx("snd_control room power down vrs3", true, 1, 60);
        Sfx("snd_Bin-Met_28G_HD2-28022", false, 12, 60);
        m_brkPanel = 0; m_brkFill = 0.0f; m_brkFreddy = 0;
        m_brkRollT = 0.0f; m_brkAnimT = 0.0f;
    }
    m_cardT += dt;
    if (m_scareT > 0.0f) return;   // the scare override (the switch ticks it)

    // fill the current panel while A is held (the dump's 4-s fill gates)
    if (in.aHeld && m_brkPanel < 3) {
        m_brkFill += dt / 0.04f * 0.5f;
        if (m_brkFill >= 100.0f) {
            m_brkFill = 0.0f;
            m_brkPanel += 1;
            Sfx("snd_Snapmouse_1_clk_2", false, 25, 80);
        }
    } else if (m_brkFill > 0.0f) {
        m_brkFill -= dt * 8.0f;
        if (m_brkFill < 0.0f) m_brkFill = 0.0f;
    }
    // the noise roll: he advances while the task runs
    m_brkRollT += dt;
    if (m_brkRollT >= (m_brkPanel > 0 ? 3.5f : 4.0f)) {
        m_brkRollT = 0.0f;
        if ((rand() % 2) == 0 && m_brkPanel < 3) m_brkFreddy += 1;
    }
    m_brkAnimT += dt;
    if (m_brkFreddy >= 7) {
        m_scareT = 0.001f;
        m_scareImg = 2;
        Sfx("snd_scream op5-8", false, 24, 100);
    }
    // the task done: the day-plan beat and the exit route
    if (m_brkPanel >= 3) {
        if (m_scriptEvent < 85) m_scriptEvent = 0;
        SfxStop("snd_control room power down vrs3");
        GoTo(12);                                       // -> Baby's Room
    }
    if (in.bPressed && m_scriptEvent >= 85) {
        SfxStop("snd_control room power down vrs3");
        GoTo(12);
    }
}

void SLGame::TickBreakerHold(f32 dt) {
    (void)dt;
}

// ------------------------------------------------------------
// Parts and Service (frame 11): the face-button task. The Script Event
// chain (verbatim from the dump): begin at 150 after a 3 s HandUnit line
// (HandUnit_3_10 -> 151); the right cheek -> 152 (HandUnit_3_11 -> 153);
// left cheek -> 154 (HandUnit_3_12 -> 155); right eye -> 156
// (HandUnit_3_13 -> 157); the face opens (anim 12/13) -> 159
// (HandUnit_3_14 -> 160); the chin -> 161 (the module shows, faceopen ->
// 162, HandUnit_3_15 -> 163); the module click -> 164 (HandUnit_3_16
// plays, the 120 s timer arms, the Bonnie puppets spawn at the triggers);
// clicking both puppets' power units -> 166 (HandUnit_3_17 -> 168);
// S exits at 168 -> go to 11 (the dump route). A WRONG button plays
// "Clarification" and does not advance. The timer expiring = the Bonnie
// scare -> game over.
// ------------------------------------------------------------

void SLGame::TickPS(f32 dt, const SLInputs& in) {
    if (m_cardT == 0.0f) {
        Sfx("snd_control room power down vrs3", true, 1, 60);
        Sfx("snd_drips", true, 2, 50);
        m_psTimer = 0.0f;
        m_psPuppets = 0;
        m_psPuppet1 = m_psPuppet2 = false;
        m_psPup1T = m_psPup2T = 0.0f;
        m_psKill = 0;
        // night 3 starts its own chain (dump g12/g14: night 2 := 3)
        if (m_night < 3) m_night = 3;
        if (m_scriptEvent < 150) m_scriptEvent = 150;
    }
    m_cardT += dt;
    if (m_scareT > 0.0f) return;

    // the scripted HandUnit cadence: each beat holds ~1-3 s then advances
    // (the sample names map to the dump's beats; the mp3 voices are silent
    // on the 360 pak — the beats still gate the task, labeled)
    m_stepSndT += dt;
    switch (m_scriptEvent) {
        case 150: if (m_stepSndT >= 3.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_10", false, 24, 100); m_scriptEvent = 151; } break;
        case 152: if (m_stepSndT >= 1.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_11", false, 24, 100); m_scriptEvent = 153; } break;
        case 154: if (m_stepSndT >= 1.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_12", false, 24, 100); m_scriptEvent = 155; } break;
        case 156: if (m_stepSndT >= 1.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_13", false, 24, 100); m_scriptEvent = 157; } break;
        case 159: if (m_stepSndT >= 2.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_14", false, 24, 100); m_scriptEvent = 160; } break;
        case 162: if (m_stepSndT >= 1.0f) { m_stepSndT = 0.0f;
                      Sfx("snd_HandUnit_3_15", false, 24, 100); m_scriptEvent = 163; } break;
        default: break;
    }

    // the button sequence: the D-pad picks the button, A clicks it. The
    // dump gates each expected button by the Script Event value.
    if (in.aPressed) {
        // the picked button cycles 0..4 with the D-pad (cheek R/L, eye R,
        // nose, chin) — wrong picks play "Clarification" (dump g17)
        i32 expected = -1;
        if (m_scriptEvent == 151) expected = 0;   // right cheek
        if (m_scriptEvent == 153) expected = 1;   // left cheek
        if (m_scriptEvent == 155) expected = 2;   // right eye
        if (m_scriptEvent == 157) expected = 3;   // the nose (the face opens)
        if (m_scriptEvent == 160) expected = 4;   // the chin
        if (expected >= 0) {
            if (m_psPick == expected) {
                Sfx("snd_Snapmouse_1_clk_2", false, 25, 100);
                if (m_scriptEvent == 151) m_scriptEvent = 152;
                else if (m_scriptEvent == 153) m_scriptEvent = 154;
                else if (m_scriptEvent == 155) m_scriptEvent = 156;
                else if (m_scriptEvent == 157) {
                    Sfx("snd_faceopen2", false, 25, 100);
                    m_scriptEvent = 159;              // the face-open beat
                } else if (m_scriptEvent == 160) {
                    Sfx("snd_faceopen", false, 21, 100);
                    m_scriptEvent = 161;
                    // g28: the face opens -> 162 (the HandUnit_3_15 beat)
                    m_scriptEvent = 162;
                }
            } else {
                Sfx("snd_Clarification", false, 24, 100);
            }
        } else if (m_scriptEvent == 163 && m_psPick == 5) {
            // the module (the 6th pick): the chain's pivot (g30/g33)
            Sfx("snd_SwitchPlastic_S08FO_2518", false, 23, 100);
            m_scriptEvent = 164;
            m_psTimer = 120.0f;                        // g63: the 120 s threat
            m_psPuppets = 1;                           // ready to spawn
            Sfx("snd_HandUnit_3_16", false, 24, 100);
        } else if (m_scriptEvent == 164) {
            // clicking a puppet's power unit ends it (g66/g67): the two
            // puppets track the peeks; the pick 7/8 = unit 1/2
            if (m_psPick == 7 && m_psPuppet1) {
                m_psPuppet1 = false;
                Sfx("snd_SwitchPlastic_S08FO_2518", false, 26, 100);
            } else if (m_psPick == 8 && m_psPuppet2) {
                m_psPuppet2 = false;
                Sfx("snd_SwitchPlastic_S08FO_2518", false, 26, 100);
            } else {
                Sfx("snd_Clarification", false, 24, 100);
            }
            if (!m_psPuppet1 && !m_psPuppet2 && m_scriptEvent == 164) {
                m_scriptEvent = 166;
                Sfx("snd_HandUnit_3_17", false, 24, 100);
            }
        }
    }
    // the pick cycles with the D-pad (0..8: the five face buttons, the
    // module, then the two puppet units and a spare)
    if (in.rightPressed) m_psPick = (m_psPick + 1) % 9;
    if (in.leftPressed)  m_psPick = (m_psPick + 8) % 9;

    // the puppet threat: while the 120 s timer runs, they appear at the
    // spawn triggers; the picker must click their units before the timer
    // expires (the dump: 4 ticks per spawn window, hide 100 ticks)
    if (m_scriptEvent >= 164 && m_scriptEvent < 166) {
        m_psTimer -= dt;
        if (m_psTimer <= 0.0f) {
            m_psKill = 1;                              // g58: got you
            m_scareT = 0.001f;
            m_scareImg = 3;                            // the Bonnie scare
            Sfx("snd_scream op5-8", false, 24, 100);
        } else {
            // the puppets come alive in the last stretch
            if (m_psTimer < 60.0f && !m_psPuppet1) { m_psPuppet1 = true; m_psPup1T = 100.0f; }
            if (m_psTimer < 30.0f && !m_psPuppet2) { m_psPuppet2 = true; m_psPup2T = 100.0f; }
        }
    }
    if (m_scriptEvent == 166 && m_stepSndT >= 1.0f) {
        m_scriptEvent = 167;                           // HandUnit_3_17 -> 167
        Sfx("snd_HandUnit_3_17", false, 24, 100);
    }
    if (m_scriptEvent == 167 && m_stepSndT >= 1.0f) {
        m_scriptEvent = 168;                           // the exit unlock
    }
    // S exits (dump g7: key S + Script Event == 168 -> go to 11)
    if (in.bPressed && m_scriptEvent >= 168) {
        SfxStop("snd_control room power down vrs3");
        SfxStop("snd_drips");
        GoTo(11);                                      // -> Breaker via load
    }
}

// ------------------------------------------------------------
// Under Desk (frame 13): the Bidybab eye-match. Peek 1 plays
// "hello_in_there"; the player keeps the pointer on her face spot while
// the sequence runs; losing her (the eye-match breaks) or timing out =
// the jumpscare -> game over. Exits: the dump's fade -> Elevator.
// ------------------------------------------------------------

void SLGame::TickDesk(f32 dt, const SLInputs& in) {
    if (m_cardT == 0.0f) {
        Sfx("snd_hello_in_there", false, 3, 100);
        m_deskHold = 0.0f;
        m_deskTotal = 0.0f;
        m_deskPeek = false;
    }
    m_cardT += dt;
    if (m_scareT > 0.0f) return;

    // the eye-match: the pointer (the D-pad nudges it) tracks her; the
    // hold fills while she is matched (A held = the steady hand)
    m_deskTotal += dt;
    if (in.aHeld) {
        m_deskHold += dt;
        if (!m_deskPeek) {
            m_deskPeek = true;
            Sfx("snd_Bidy", false, 4, 100);
        }
    } else {
        m_deskHold -= dt * 0.5f;
        if (m_deskHold < 0.0f) m_deskHold = 0.0f;
        m_deskPeek = false;
    }
    // she darts: every 2 s a 1-in-3 slip; falling too far behind = the scare
    m_moveT += dt;
    if (m_moveT >= 2.0f) {
        m_moveT = 0.0f;
        if ((rand() % 3) == 0 && m_deskHold > 1.0f) m_deskHold -= 2.0f;
    }
    if (m_deskHold < -1.5f || (m_deskTotal > 45.0f && m_deskHold < 4.0f)) {
        m_scareT = 0.001f;
        m_scareImg = 4;                                // the Bidybab scare
        Sfx("snd_scream op5-8", false, 24, 100);
    }
    // the sequence ends: 25 s of holding -> the dump's fade -> Elevator
    if (m_deskHold >= 25.0f || m_deskTotal >= 60.0f) {
        SfxStop("snd_drips");
        GoTo(1);                                       // -> Elevator
    }
}

// ------------------------------------------------------------
// Extras (frame 18): the dump's viewer rows (the jumpscare player + the
// unlocked entries). Wave 2 keeps the shell: the rows, the pick, and the
// jumpscare playback; the per-row media is wave 3.
// ------------------------------------------------------------

void SLGame::TickExtras(f32 dt, const SLInputs& in) {
    (void)dt;
    if (in.upPressed)   { m_optionSelected = (m_optionSelected + 3) % 4; Sfx("snd_select3", false, 3, 80); }
    if (in.downPressed) { m_optionSelected = (m_optionSelected + 1) % 4; Sfx("snd_select3", false, 3, 80); }
    if (in.leftPressed || in.rightPressed) {
        m_dirSide = (m_dirSide + 1) & 1;
        Sfx("snd_select3", false, 3, 80);
    }
    if (in.aPressed) {
        // the jumpscare viewer row plays a scare for a beat
        if (m_optionSelected == 1) {
            m_scareT = 0.001f;
            m_scareImg = 3;
            Sfx("snd_scream op5-8", false, 3, 100);
            return;
        }
        if (m_optionSelected == 3 || in.bPressed) {
            m_screen = SCR_TITLE;
            return;
        }
    }
    if (in.bPressed) {
        m_screen = SCR_TITLE;
    }
}

// ------------------------------------------------------------
// the night-4/5 end chain: Scooping Room (fade to red) -> red fade out
// (end words := 1) -> bathroom (the mirror) -> credits; the night-5
// Final Encounter routes here too. The dump's frame-21 red fade feeds the
// P&S 2 chain; the port walks the screens with their hold beats.
// ------------------------------------------------------------

void SLGame::TickChain(f32 dt, const SLInputs& in) {
    if (m_cardT == 0.0f) {
        if (m_screen == SCR_SCOOPING) {
            Sfx("snd_ScoopingRoom-02", true, 1, 60);
        }
    }
    m_cardT += dt;
    if (in.bPressed || m_cardT >= 8.0f) {
        m_cardT = 0.0f;
        SfxStop("snd_ScoopingRoom-02");
        switch (m_screen) {
            case SCR_SCOOPING: m_screen = SCR_REDFADE; break;   // frame 21
            case SCR_REDFADE:  m_endsceneno = 1;                // "end words = 1"
                               m_saveDirty = true;
                               m_screen = SCR_BATHROOM; break;  // frame 22
            case SCR_BATHROOM: m_screen = SCR_CREDITS; break;   // frame 23
            default:
                // the credits end: the night-5 loop parks at the title
                m_screen = SCR_TITLE;
                m_started = false;
                break;
        }
    }
}

// ------------------------------------------------------------
// Baby's Room (frame 6): the night ends here — the dump exits the night on
// the fade-out -> win night (fade > 255). Playing the room = the night.
// ------------------------------------------------------------

void SLGame::TickBaby(f32 dt, const SLInputs& in) {
    // the night-1 speech (the first-launch monologue cutscene rides
    // separately); wave 1: stand a beat then the fade -> win night
    (void)in;
    m_cardT += dt;
    if (m_cardT >= 8.0f || in.bPressed) {
        m_cardT = 0.0f;
        SfxStop("snd_hub hum");
        m_screen = SCR_WINNIGHT;
        m_winT = 0.0f;
        Sfx("snd_Jingle_4b", false, 1, 100);            // the win jingle
    }
}

// ------------------------------------------------------------
// win night (frame 12): Jingle_4b + the INI write (current = night+1) ->
// tv show -> Girl Voice
// ------------------------------------------------------------

void SLGame::TickWinNight(f32 dt) {
    m_winT += dt;
    if (m_winT >= 4.0f) {
        m_winT = 0.0f;
        m_current = m_night + 1;
        if (m_current > 5) m_current = 5;
        m_night = m_current;
        m_saveDirty = true;
        SfxStop("snd_Jingle_4b");
        // the checkmark beat, then the tv show
        m_screen = SCR_TVSHOW;
        m_tvT = 0.0f;
        Sfx("snd_tv show", true, 2, 15);
    }
}

void SLGame::TickTvShow(f32 dt) {
    m_tvT += dt;
    if (m_tvT >= 8.0f || true) {
        m_tvT = 0.0f;
        SfxStop("snd_tv show");
        m_screen = SCR_GIRLVOICE;
        m_girlT = 0.0f;
        // the girl says a line per night (line_1..6); the sample name
        switch (m_night) {
            case 1: Sfx("snd_girl line3", false, 4, 100); break;
            default: Sfx("snd_girl line4", false, 4, 100); break;
        }
    }
}

void SLGame::TickGirlVoice(f32 dt) {
    m_girlT += dt;
    if (m_girlT >= 6.0f) {
        m_girlT = 0.0f;
        if (m_night >= 5) {
            // the final route: red fade -> the P&S 2 chain (wave 2 hold)
            m_screen = SCR_TITLE;
            m_started = false;
        } else {
            m_screen = SCR_TITLE;                       // interlude end = next
        }
    }
}

// ------------------------------------------------------------
// death (frame 14) — the game's "planned death" respawn router
// ------------------------------------------------------------

void SLGame::TickDeath(f32 dt, const SLInputs& in) {
    (void)dt;
    // game over -> the resume presets per night (docs §6: n2 -> 81,
    // n3 -> 111, n4 -> 57 + intro, n5 -> 512)
    if (in.aPressed || m_cardT >= 5.0f) {
        m_cardT = 0.0f;
        static const i32 kResume[5] = { 81, 111, 57, 512 };
        m_scriptEvent = kResume[m_current < 2 ? 0 : (m_current > 5 ? 3 : m_current - 2)];
        if (m_current >= 4) m_intro = true;             // the night-4 marker
        NightStart();
    }
    m_cardT += dt;
}

void SLGame::TickGameOver(f32 dt, const SLInputs& in) {
    // frame 15: caught -> hold; A -> back to the resume
    m_cardT += dt;
    if (in.aPressed || in.bPressed || m_cardT >= 5.0f) {
        m_cardT = 0.0f;
        m_screen = SCR_DEATH;
    }
}

} // namespace fnaf
