/**
 * FNaF4Game.cpp: v2.61 — FNAF4 night loop (see FNaF4Game.h).
 * Dump authority: Dumps/Five Nights at Freddys 4/Events/frame_3_level_events.txt
 * (480 groups). Timers are RAW MILLISECONDS (60 s hours, 5 s lane rolls,
 * 3 s shut-door visits, 10 s retreats, 4 s bed ticks). Labeled deviations:
 * the walk-through-the-hall free movement is collapsed to five positions
 * (console pad, no cursor); night 5's Bonnie/Chica/Freddy/Foxy AI carry
 * night 4's table (the dump only pins Fredbear 12).
 */

#include "FNaF4Game.h"
#include <stdlib.h>

namespace fnaf {

FNaF4Game::FNaF4Game() {
    ResetToTitle();
}

void FNaF4Game::ResetToTitle() {
    m_screen = SCR_DISCLAIMER;
    m_night = 1;
    m_hour = 0;
    m_hourClock = 0.0f;
    m_cardT = 0.0f;
    m_time = 0.0f;
    m_optionSelected = 0;
    m_lastNight = 1;
    m_beat5 = false;
    m_pos = P_CENTER;
    m_walkT = 0.0f;
    m_peeking = false;
    m_leftShut = m_rightShut = false;
    m_listening = 0;
    m_gameover = false;
    m_attackImg = 0;
    m_attackT = 0.0f;
    m_biteT = 0.0f;
    // v2.64: the fn4 state
    m_scene = 0;
    m_beat6 = m_beat7 = m_beat8 = false;
    m_s1 = m_s2 = m_s3 = m_s4 = m_s5 = m_s6 = false;
    m_testFlag = false;
    m_shadow = 0;
    m_minigamePlay = false;
    m_cheatHouseMap = m_fastNights = m_cheatRadar = false;
    m_blindMode = m_instaFoxy = m_madFreddy = m_allNightmare = false;
    m_pt.Clear();
    m_cut.Clear();
    m_ex.Clear();
    for (i32 i = 0; i < 4; ++i) { m_winDigit[i] = 0; m_winVal[i] = 0; }
    m_digitT = 0.0f; m_winT = 0.0f; m_lockT = 0.0f;
    m_introT = 0.0f; m_goT = 0.0f; m_endT = 0.0f; m_endLine = 0; m_endLetterT = 0.0f;
    m_saveDirty = false;
    m_ptFlashPrev = false;
    m_lockLid = 0.0f;
}

void FNaF4Game::StartNight(i32 night) {
    m_night = night;
    InitNightState();
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
}

void FNaF4Game::InitNightState() {
    m_hour = 0;
    m_hourClock = 0.0f;
    m_pos = P_CENTER;
    m_walkT = 0.0f;
    m_walkDir = 0;
    m_peeking = false;
    m_leftShut = m_rightShut = false;
    m_prevLeftShut = m_prevRightShut = false;
    m_listening = 0;

    m_bonniePos = m_chicaPos = 0;
    m_laneRollT = 0.0f;
    m_bonnieIgnoreT = m_chicaIgnoreT = 0.0f;
    m_bonnieNearT = m_chicaNearT = 0.0f;
    m_bonnieBed = m_chicaBed = false;
    m_bonnieVisitT = m_chicaVisitT = 0.0f;

    m_foxyIn = false;
    m_closetCounter = 0;
    m_foxyGotYou = 0;
    m_foxyRollT = 0.0f;
    m_closetTickT = 0.0f;
    m_fredbearSndT = 0.0f;
    // the cheat toggles pre-arm the night where the dump would (Extras):
    // blind = start with the flashlight dark, mad_freddy = Freddy already
    // warm, insta_foxy = Foxy in the closet from 12 AM, all_nightmare =
    // Fredbear replaces everyone from hour 0. house map/radar handled by the
    // renderer's overlay (labeled approximation of the excluded objects)
    if (m_instaFoxy) { m_closetCounter = 3; }
    if (m_allNightmare && m_fredbearAI == 0) m_fredbearAI = 12;
    if (m_madFreddy)   m_freddyCounter = 20;
    m_miniArt = rand() % 4;
    m_freddyT = 0.0f;
    m_bedTickT = 0.0f;
    m_biteT = 0.0f;

    m_totalDanger = 0;
    m_dangerT = 0.0f;
    m_bfTimer = 5.0f;
    m_blackFlashA = 0.0f;
    m_bfHold = 0.0f;

    m_attackImg = 0;
    m_attackT = 0.0f;
    m_gameover = false;

    ApplyNightAI();
}

// ------------------------------------------------------------
// helpers
// ------------------------------------------------------------

void FNaF4Game::Sfx(const char* s, bool loop, i32 ch, i32 vol) {
    if (audio.play) audio.play(s, loop, ch, vol);
}
void FNaF4Game::SfxStop(const char* s) {
    if (audio.stop) audio.stop(s);
}
void FNaF4Game::ChVol(i32 ch, i32 vol) {
    if (audio.channelVolume) audio.channelVolume(ch, vol);
}

// the per-night/hour AI tables (dump groups 387-409 + 111/444/405)
void FNaF4Game::ApplyNightAI() {
    switch (m_night) {
        case 1:
            m_bonnieAI = m_chicaAI = m_freddyAI = 0; m_foxyAI = 0;
            if (m_hour >= 2) { m_bonnieAI = 1; m_chicaAI = 1; m_freddyAI = 1; }
            if (m_hour >= 3) { m_bonnieAI = 3; m_chicaAI = 2; m_freddyAI = 2; }
            break;
        case 2:
            m_bonnieAI = 5; m_chicaAI = 5; m_freddyAI = 2; m_foxyAI = 1;
            if (m_hour >= 3) { m_bonnieAI = 7; m_chicaAI = 7; m_freddyAI = 3; m_foxyAI = 4; }
            break;
        case 3:
            m_bonnieAI = 7; m_chicaAI = 7; m_freddyAI = 3; m_foxyAI = 10;
            if (m_hour >= 3) { m_bonnieAI = 10; m_chicaAI = 10; }
            break;
        case 4:
            m_bonnieAI = 10; m_chicaAI = 10; m_freddyAI = 4; m_foxyAI = 5;
            if (m_hour >= 3) { m_bonnieAI = 12; m_chicaAI = 12; m_foxyAI = 10; }
            break;
        case 5:
            // DEVIATION: the dump pins only Fredbear 12 for night 5; the
            // rest carries night 4's table
            m_bonnieAI = 10; m_chicaAI = 10; m_freddyAI = 4; m_foxyAI = 5;
            m_fredbearAI = 12;
            if (m_hour >= 3) { m_bonnieAI = 12; m_chicaAI = 12; m_foxyAI = 10; }
            break;
        default:   // night 6+
            m_bonnieAI = 12; m_chicaAI = 12; m_freddyAI = 5; m_foxyAI = 10;
            m_fredbearAI = 0;
            if (m_hour >= 4) {   // g405: Fredbear takes the whole house
                m_bonnieAI = 0; m_chicaAI = 0; m_freddyAI = 0; m_foxyAI = 0;
                m_fredbearAI = 15;
            }
            break;
    }
    if (m_night < 5) m_fredbearAI = 0;
    if (m_fredbearAI > 0) m_freddyCounter = 0;   // g433
}

void FNaF4Game::TriggerJumpscare(i32 overlayImg) {
    if (m_gameover) return;
    m_attackImg = overlayImg;
    m_attackT = 0.001f;
    m_gameover = true;
    Sfx("snd_scream2", false, 16, 100);
}

// ------------------------------------------------------------
// tick
// ------------------------------------------------------------

void FNaF4Game::Tick(f32 dt, const FNaF4Inputs& in) {
    m_time += dt;

    switch (m_screen) {
        case SCR_DISCLAIMER:
            m_cardT += dt;
            if (m_cardT >= 3.5f || in.aHeld || in.bPressed) {
                m_screen = SCR_TITLE; m_cardT = 0.0f;
            }
            break;

        case SCR_TITLE:
            // 4 dump rows (new game / continue / 6th night / extras).
            // g34/g43: the play rows enter the CUTSCENES frame (the intro
            // scene 0 from new game, the scene = the night from continue);
            // g47/g55: the extras row -> V9 = slot 9 = extras.
            if (in.upPressed)    m_optionSelected = (m_optionSelected + 3) % 4;
            if (in.downPressed)  m_optionSelected = (m_optionSelected + 1) % 4;
            if (in.aPressed) {
                switch (m_optionSelected) {
                    case 0:                                 // new game
                        m_night = 1;
                        m_cut.Clear();
                        m_cut.scene = 0;
                        m_screen = SCR_CUTSCENE;
                        m_cardT = 0.0f;
                        Sfx("snd_Deep_Ambience_With_Sc", true, 1, 15);
                        break;
                    case 1:                                 // continue
                        m_cut.Clear();
                        m_cut.scene = m_scene;
                        m_screen = SCR_CUTSCENE;
                        m_cardT = 0.0f;
                        Sfx("snd_Deep_Ambience_With_Sc", true, 1, 15);
                        break;
                    case 2:                                 // night 6 (beat5)
                        if (m_beat5) { m_shadow = 0; GoWhatNight(6); }
                        break;
                    case 3:
                        m_ex.Clear();
                        m_screen = SCR_EXTRAS;
                        m_cardT = 0.0f;
                        Sfx("snd_Deep_Ambience_With_Sc", true, 1, 15);
                        break;
                }
            }
            break;

        case SCR_NIGHTSTART:
            TickWhatNight(dt, in);
            break;

        case SCR_BEDROOM:
            TickBedroom(dt, in);
            break;

        case SCR_NIGHTWIN:
            TickNightWin(dt, in);
            break;

        case SCR_GAMEOVER:
            TickGameOver(dt, in);
            break;

        case SCR_GAMEOVER2:
            TickGameOver2(dt);
            break;

        case SCR_INTRO:
            TickIntro(dt, in);
            break;

        case SCR_PLUSH:
        case SCR_BB:
            TickMinigame(dt, in);
            break;

        case SCR_LOCKBOX:
        case SCR_LOADX:
            TickLockbox(dt, in);
            break;

        case SCR_EXTRAS:
            TickExtras(dt, in);
            break;

        case SCR_CUTSCENE:
            TickCutscene(dt, in);
            break;

        case SCR_ENDING:
            TickEnding(dt, in);
            break;

        case SCR_TEST:
            m_cardT += dt;
            if (m_cardT >= 1.0f || in.aPressed || in.bPressed) {
                m_screen = SCR_TITLE;
                m_cardT = 0.0f;
            }
            break;

        case SCR_NJSCARE:
            m_goT += dt;
            if (m_goT >= 5.0f) {
                m_screen = SCR_TITLE;      // V11 = slot 11 = the disclaimer slot
                m_goT = 0.0f;
            }
            break;
    }

    // the jumpscare anim runs over any screen; it ends on the death flow
    if (m_attackT > 0.0f) {
        m_attackT += dt;
        if (m_attackT >= 1.5f) {                        // g204-207
            m_attackT = 0.0f;
            m_attackImg = 0;
            m_gameover = false;
            m_screen = SCR_GAMEOVER;                    // V1 = slot 1 = game over
            m_cardT = 0.0f;
        }
    }
    // the cosmetic bite overlay (g255) just fades
    if (m_biteT > 0.0f) m_biteT -= dt;
}

// ============================================================
// v2.64 — the dump's outer flow + Fun with Plushtrap/BB + the lockbox +
// the extras + the house cutscenes. Jump values = storyboard slots:
// 0 level, 1 game over, 2 what night, 3 night win, 4 title, 5 intro
// plushtrap, 6 plushtrap game, 7 lockbox, 8 game over 2, 9 extras,
// 10 load extras, 11 disclaimer, 12 Cutscenes, 13 ending, 14 test,
// 15 nightmare jumpscare, 16 demo, 19 BB game.
// ============================================================

// the real night card (frame 2 "what night"): Night := INI night; the
// shadow pick forces nights 7/8; 2 s -> the clock shows, 2.1 s -> the level
void FNaF4Game::GoWhatNight(i32 night) {
    m_night = night < 1 ? 1 : (night > 8 ? 8 : night);
    m_lastNight = m_night > 6 ? 6 : m_night;
    m_screen = SCR_NIGHTSTART;
    m_cardT = 0.0f;
    Sfx("snd_ambience", true, 1, 10);
}

void FNaF4Game::TickWhatNight(f32 dt, const FNaF4Inputs& in) {
    (void)in;
    m_cardT += dt;
    if (m_cardT >= 2.1f) {
        m_cardT = 0.0f;
        StartNight(m_night < 1 ? 1 : m_night);
        m_screen = SCR_BEDROOM;
    }
}

// the real night win (frame 5): the "6 AM" digits settle 2/2.5/3/3.5 s,
// the INI writes, then the 10 s route: night <= 5 -> Cutscenes, 6 ->
// ending, 7 -> lockbox, >= 7 challenge / >= 8 -> title (V4)
void FNaF4Game::WriteNightResult() {
    // g11: the INI night = the beaten night + 1
    m_night += 1;
    if (m_night > 8) m_night = 8;
    m_lastNight = m_night > 6 ? 6 : m_night;
    if (m_night - 1 == 5) m_beat5 = true;             // g16: beat5
    if (m_night - 1 == 6) m_beat6 = true;             // g17: beat6
    if (m_night - 1 == 7) {                           // g18: beat7 + test arm
        m_beat7 = true;
        m_testFlag = true;
    }
    if (m_night - 1 == 8 && !(m_blindMode || m_madFreddy || m_instaFoxy ||
                              m_allNightmare)) {      // g19: beat8 clean
        m_beat8 = true;
        m_testFlag = true;
    }
    // g25-30: the challenge-combo stars (blind/mad/insta/allnightmare pairs)
    if (m_night >= 7 && !(m_blindMode || m_madFreddy || m_instaFoxy || m_allNightmare)) {
        if (m_blindMode) m_s1 = true;
        if (m_madFreddy) m_s2 = true;
        if (m_instaFoxy) m_s3 = true;
        if (m_allNightmare) m_s4 = true;
        if (m_allNightmare && m_blindMode) m_s5 = true;
        if (m_blindMode && m_madFreddy && m_instaFoxy) m_s6 = true;
    }
    m_scene = m_night <= 5 ? m_night : m_scene;       // the next cutscene id
    // the shadow flag clears once used (game over g2 does the same)
    m_shadow = 0;
    m_minigamePlay = false;
    m_saveDirty = true;
}

void FNaF4Game::TickNightWin(f32 dt, const FNaF4Inputs& in) {
    (void)in;
    m_winT += dt;
    // the digit settle (g2-10): each digit flickers Random(10)/250 ms until
    // its beat (2 / 2.5 / 3 / 3.5 s), then the 4 s full-hold
    m_digitT += dt;
    if (m_digitT >= 0.25f) {
        m_digitT -= 0.25f;
        for (i32 i = 0; i < 4; ++i) {
            if (m_winDigit[i] == 0) m_winVal[i] = rand() % 10;
        }
    }
    static const f32 kBeat[4] = { 2.0f, 2.5f, 3.0f, 3.5f };
    static const i32 kSet[4] = { 0, 6, 0, 0 };        // g6-9 (num2 = 6 = AM)
    for (i32 i = 0; i < 4; ++i) {
        if (m_winDigit[i] == 0 && m_winT >= kBeat[i]) {
            m_winDigit[i] = 1;
            m_winVal[i] = kSet[i];
        }
    }
    if (m_winT >= 10.0f) {
        m_winT = 0.0f;
        WriteNightResult();
        if (m_night <= 5) {
            // g12: -> Cutscenes (scene = the beaten night)
            m_cut.Clear();
            m_cut.scene = m_night <= 4 ? m_night : 4;
            m_screen = SCR_CUTSCENE;
            Sfx("snd_Deep_Ambience_With_Sc", true, 1, 15);
        } else if (m_night == 6) {
            m_screen = SCR_ENDING;                    // g20: the ending
            m_endT = 0.0f; m_endLine = 0; m_endLetterT = 0.0f;
        } else if (m_night == 7) {
            m_screen = SCR_LOCKBOX;                   // g21 (V7 = the lockbox)
            m_lockT = 0.0f;
        } else {
            m_screen = SCR_TITLE;                     // g22/23: V4 = title
        }
    }
}

void FNaF4Game::TickGameOver(f32 dt, const FNaF4Inputs& in) {
    // frame 4: hide, stop all, shadow := 0, 7 s -> title (V4)
    if (m_cardT == 0.0f) {
        m_shadow = 0;
        m_saveDirty = true;
    }
    m_cardT += dt;
    if (m_cardT >= 7.0f || in.aPressed) {
        m_cardT = 0.0f;
        m_screen = SCR_TITLE;
    }
}

void FNaF4Game::TickGameOver2(f32 dt) {
    // frame 8: 4 s -> what night (normal) / load extras -> lockbox (replay)
    m_cardT += dt;
    if (m_cardT >= 4.0f) {
        m_cardT = 0.0f;
        if (!m_minigamePlay) {
            GoWhatNight(m_night);                     // V2: retry the night
        } else {
            m_screen = SCR_LOCKBOX;                   // V10 -> V9 lockbox
            m_lockT = 0.0f;
        }
    }
}

void FNaF4Game::TickIntro(f32 dt, const FNaF4Inputs& in) {
    // frames 6/17: Deep_Ambience, 6 s / A -> Next (the minigame)
    if (m_cardT == 0.0f) {
        SfxStop("snd_Deep_Ambience_With_Sc");
        Sfx("snd_Deep_Ambience_With_Sc", true, 1, 15);
    }
    m_cardT += dt;
    if (m_cardT >= 6.0f || in.aPressed || in.bPressed) {
        m_cardT = 0.0f;
        StartMinigame(m_pt.game, m_minigamePlay);
    }
}

// ---- Fun with Plushtrap (frame 7) / Fun with BB (frame 18) ----
// One engine: the hall graph in chair -> stage 1 -> (fork) far left/right
// -> stage 2 -> (fork) close left/right -> stage 3 -> got you. A hold =
// the flash. Flashing at stage 3 (after the dark beat) = the win; he
// reaches "got you" and you flash = the jumpscare; the clock runs out =
// caught. The view anims and the darkness accumulate exactly per dump.

void FNaF4Game::StartMinigame(i32 game, bool fromExtras) {
    m_pt.Clear();
    m_pt.game = game;
    m_pt.fromExtras = fromExtras;
    m_pt.clock = game == 1 ? 45 :                     // g53 (BB): 45 s
        (m_night <= 1 ? 90 : m_night == 2 ? 60 : m_night == 3 ? 45 : 30);
    m_screen = game == 1 ? SCR_BB : SCR_PLUSH;
    m_cardT = 0.0f;
    SfxStop("snd_Deep_Ambience_With_Sc");
    // g43: crickets ch2 (10) + ch3 (30) + ch5 (100)
    Sfx("snd_crickets", true, 2, 10);
    Sfx("snd_crickets", true, 3, 30);
    Sfx("snd_crickets", true, 5, 100);
}

void FNaF4Game::TickMinigame(f32 dt, const FNaF4Inputs& in) {
    PtState& pt = m_pt;
    const bool flash = in.aHeld;                       // the Ctrl key
    static const i32 kForkPos[2] = { 2, 3 };           // far left / far right

    if (in.bPressed) {
        // the dump's Escape quits; the console abandons to the card/lockbox
        SfxStop("snd_crickets");
        if (pt.fromExtras) { m_screen = SCR_LOCKBOX; m_lockT = 0.0f; }
        else                 GoWhatNight(m_night);
        return;
    }

    // the flash edge sounds (g44/48)
    if (flash != m_ptFlashPrev) {
        Sfx("snd_FLASHLIG_7_GEN-HDF11244", false, 3, 100);
        m_ptFlashPrev = flash;
        if (flash) pt.viewState = 1;                   // g42: the look state
        else if (pt.viewState == 1) pt.viewState = 0;  // g1: back to idle
    }
    if (flash) pt.darkT = 0.0f;                        // g41

    // the win/jumpscare latches
    if (pt.viewState == 99) {
        pt.viewAnim = 23;                              // g46: the jumpscare
        pt.scare = true;
        Sfx("snd_scream3", false, 4, 100);
    }
    if (pt.scare) {
        m_cardT += dt;
        if (m_cardT >= 1.5f) {
            SfxStop("snd_crickets");
            m_screen = SCR_GAMEOVER2;                  // g49: -> game over 2
            m_cardT = 0.0f;
        }
        return;
    }

    // the darkness accumulates only in the dark (g9), drives the move roll
    if (!flash) pt.darkT += dt;
    pt.moveGateT += dt;
    if (pt.moveGateT >= 2.0f && !flash && pt.darkT * 60.0f >= 400.0f) {
        pt.moveGateT = 0.0f;
        pt.darkT = 200.0f / 60.0f;
        pt.hallPos += 1;                               // one hall step
        pt.fork = 1 + rand() % 2;
        static const char* const kStep[8] = {
            "snd_quickwalk1", 0, 0, 0, "snd_quickwalk2", 0, 0, "snd_quickwalk3"
        };
        if (pt.hallPos == 8) pt.hallPos = 8;           // got you — waits
        if (kStep[pt.hallPos - 1]) Sfx(kStep[pt.hallPos - 1], false, 3, 100);
        else Sfx((pt.fork == 1) ? "snd_right to left fastb"
                                : "snd_left to right fastb", false, 3, 100);
        if (pt.hallPos == 7) {                         // g61/55: stage 3
            if (pt.game == 1) {                        // g62: the BB taunt
                pt.bbVoiceT = 0.001f;
            }
        }
    }
    // BB: the 500 ms extra move roll at stage 3 (g45) + the voice lines
    if (pt.game == 1) {
        pt.stepT += dt;
        if (pt.stepT >= 0.5f) {
            pt.stepT = 0.0f;
            if (pt.hallPos == 7 && !flash) pt.hallPos = 8;
        }
        pt.bbVoiceT += dt;
        if (pt.bbVoiceT >= 2.0f) {
            pt.bbVoiceT = 0.0f;
            if ((rand() % 3) == 0) {
                static const char* const kBb[3] = { "snd_bb1b", "snd_bb2b", "snd_bb3b" };
                Sfx(kBb[rand() % 3], false, 8, 10);
            }
        }
    }

    // the flash resolution (g21 + g28-39): stage 1 -> he jumps back; the
    // far/close rooms -> the visible poses; stage 3 held -> the WIN
    if (flash && pt.viewState == 1) {
        if (pt.hallPos == 1) {
            pt.hallPos = 0;                            // g21: back to the chair
            pt.viewState = 2;                          // the jumped-back pose
            pt.viewAnim = 14;
            Sfx("snd_quickwalk3", false, 3, 100);
        } else if (pt.hallPos == 4 || pt.hallPos == 7) {
            Sfx("snd_drop", false, 5, 100);
            if (pt.hallPos == 7) {
                pt.viewState = 98;                     // g39: the win!
                pt.viewAnim = 22;
                pt.won = true;
                pt.winT = 0.0f;
                if (pt.game == 1) m_s3 = true;         // BB reward (s3 star)
            }
        } else if (pt.hallPos >= 2 && pt.hallPos <= 6) {
            pt.viewState = 3;                          // the visible poses
            pt.viewAnim = 15 + (pt.hallPos - 2);
        }
    }
    // the pose timers fall back to the look state (g34-38)
    if (pt.viewState == 3 && !flash) pt.viewState = 1;

    // the got-you flash (g46): flashing while he is AT the chair = caught
    if (flash && pt.hallPos == 8) {
        pt.viewState = 99;
    }

    // the clock (g54-60): 1 s decrement, <= 0 -> caught
    pt.clockT += dt;
    if (pt.clockT >= 1.0f && !pt.won) {
        pt.clockT -= 1.0f;
        pt.clock -= 1;
        if (pt.clock <= 0) {
            pt.scare = true;
            pt.viewState = 99;
            pt.viewAnim = 23;
            Sfx("snd_scream3", false, 4, 100);
        }
    }

    // the win hold (g50-52: 150 frames of the anim-22 view) then the exit
    if (pt.won) {
        pt.winT += dt * 60.0f;
        if (pt.winT >= 150.0f) {
            SfxStop("snd_crickets");
            if (pt.game == 0) {
                if (pt.fromExtras) { m_screen = SCR_LOCKBOX; m_lockT = 0.0f; }
                else                 GoWhatNight(m_night);   // g51: V2
            } else {
                m_screen = SCR_LOCKBOX;                // g51 BB: -> extras slot
                m_lockT = 0.0f;
            }
        }
    }
}

// ---- the lockbox (frame 9): after night 7; A unlocks, the lid floats ----

void FNaF4Game::TickLockbox(f32 dt, const FNaF4Inputs& in) {
    if (m_screen == SCR_LOADX) {                       // frame 11: instant
        m_screen = SCR_LOCKBOX;
        m_lockT = 0.0f;
        Sfx("snd_Deep_Ambience_With_Sc", true, 2, 15);
        return;
    }
    if (m_lockT == 0.0f) {
        SfxStop("snd_Deep_Ambience_With_Sc");
        Sfx("snd_Deep_Ambience_With_Sc", true, 2, 15);
    }
    m_lockT += dt;
    if (in.bPressed) {                                 // Escape -> end
        SfxStop("snd_Deep_Ambience_With_Sc");
        m_screen = SCR_TITLE;
        return;
    }
    if (in.aPressed && m_lockLid < 1.0f) {
        m_lockLid = 0.001f;                            // g3/g4: the unlock anim
        Sfx("snd_unlock2", false, 1, 100);
    }
    if (m_lockLid > 0.0f) {
        m_lockLid += dt;
        if (m_lockLid >= 2.0f) m_lockLid = 2.0f;       // the lid is off
    }
    if (m_lockT >= 20.0f) {                            // g10: the auto-end
        SfxStop("snd_Deep_Ambience_With_Sc");
        m_screen = SCR_TITLE;
    }
}

// ---- the extras (frame 10) ----

void FNaF4Game::TickExtras(f32 dt, const FNaF4Inputs& in) {
    (void)dt;
    if (in.upPressed)   { m_ex.row = (m_ex.row + 9) % 10; Sfx("snd_select3", false, 3, 80); }
    if (in.downPressed) { m_ex.row = (m_ex.row + 1) % 10; Sfx("snd_select3", false, 3, 80); }
    if (in.leftPressed || in.rightPressed) {
        const i32 d = in.rightPressed ? 1 : -1;
        m_ex.pick = (m_ex.pick + 8 + d) % 8;
        Sfx("snd_select3", false, 3, 80);
    }
    if (in.bPressed) {
        SfxStop("snd_Deep_Ambience_With_Sc");
        SfxStop("snd_scream2");
        m_screen = SCR_TITLE;
        return;
    }
    if (in.aPressed) {
        switch (m_ex.row) {
            case 4:                                    // fun with plushtrap
                m_minigamePlay = true;
                m_pt.game = 0;
                m_screen = SCR_INTRO;
                m_cardT = 0.0f;
                break;
            case 5:                                    // the shadow nights
                m_shadow = (m_ex.pick % 2) + 1;        // 1 = night 7, 2 = night 8
                SfxStop("snd_Deep_Ambience_With_Sc");
                GoWhatNight(m_shadow == 1 ? 7 : 8);    // g89-92 -> V2, Night 7/8
                break;
            case 8:                                    // fun with BB
                m_minigamePlay = true;
                m_pt.game = 1;
                m_screen = SCR_INTRO;
                m_cardT = 0.0f;
                break;
            case 9:                                    // exit
                SfxStop("snd_Deep_Ambience_With_Sc");
                m_screen = SCR_TITLE;
                break;
            case 3:                                    // the jumpscare player
                Sfx((m_ex.pick % 4) < 3 ? "snd_scream2" : "snd_scream3",
                    false, 3, 100);
                break;
            case 6:                                    // the cheat toggles
                switch (m_ex.pick % 8) {
                    case 0: m_cheatHouseMap = !m_cheatHouseMap; break;
                    case 1: m_fastNights = !m_fastNights; break;
                    case 2: m_cheatRadar = !m_cheatRadar; break;
                    case 3: m_blindMode = !m_blindMode; break;
                    case 4: m_instaFoxy = !m_instaFoxy; break;
                    case 5: m_madFreddy = !m_madFreddy; break;
                    case 7: m_allNightmare = !m_allNightmare; break;
                    default: break;
                }
                m_saveDirty = true;
                Sfx("snd_select3", false, 3, 80);
                break;
            default:
                Sfx("snd_select3", false, 3, 80);
                break;
        }
    }
}

// ---- the house cutscenes (frame 12): the walkable 5120x3840 world.
// v2.66g: THE DIALOGUE IS RECOVERED — the strings live in the frame-12
// events (act #88 "set paragraph text" per group), with the speaker color
// per line (act #83). The script below is verbatim; the scenes: 0 = the
// first-launch bedroom (g49-53), 1 = the hide-and-seek (g81-93), 2 = the
// plush/party day (g95-115), 3 = the closet (g117-133), 5 = the birthday
// finale (g143-154). Scene routing: 0 -> what night, 1-4 -> intro to
// plushtrap, > 4 -> title. ----

namespace {

struct CsLine { i32 scene; const char* text; u32 color; };
static const CsLine kCsScript[] = {
    // scene 0 — the bedroom (the brother's lines, yellow)
    { 0, "What did he do this time?", 0xFFFFFF57 },
    { 0, "He locked you in your room again.", 0xFFFFFF57 },
    { 0, "Don't be scared. I am here with you.", 0xFFFFFF57 },
    { 0, "Tomorrow is another day.", 0xFFFFFF57 },
    // scene 1 — the hide-and-seek (yellow)
    { 1, "You know he is hiding again.", 0xFFFFFF57 },
    { 1, "He won't stop until you find him.", 0xFFFFFF57 },
    { 1, "Over there.", 0xFFFFFF57 },
    { 1, "He left without you.", 0xFFFFFF57 },
    { 1, "He knows that you hate it here.", 0xFFFFFF57 },
    { 1, "You are right beside the exit. If you run, you can make it.", 0xFFFFFF57 },
    { 1, "Hurry, run toward the exit.", 0xFFFFFF57 },
    { 1, "Be careful.", 0xFFFFFF57 },
    // scene 2 — the plush toy / the party (green girl, orange bully, pink girl)
    { 2, "Where is your plush toy? Mine is Spring Bonnie.", 0xC0FFA0 },
    { 2, "My Daddy says I have to be careful with him or I will pinch my finger.", 0xC0FFA0 },
    { 2, "He is a finger trap, he says.", 0xC0FFA0 },
    { 2, "Why are you crying? Don't you like my toy collection?", 0xFFC0A0 },
    { 2, "Aren't you the kid who always hides under the table and cries?", 0xC0FFA0 },
    { 2, "Hahaha! No one else is scared! Why are you? Stop being such a baby!", 0xC0FFA0 },
    { 2, "Are you going to the party? Everyone is going to the party.", 0xFFA0FF },
    { 2, "Oh wait, you have to go! It's YOUR birthday! Haha!", 0xFFA0FF },
    { 2, "You'd better watch out! I hear they come to life at night.", 0xFFFFA0 },
    { 2, "And if you die, they hide your body and never tell anyone.", 0xFFFFA0 },
    { 2, "Why do you look so worried? See you at the party! Ha ha ha!", 0xFFFFA0 },
    // scene 3 — the escape / the closet (yellow, then the child's white)
    { 3, "NO! Don't you remember what you saw? The exit is the other way!", 0xFFFFFF57 },
    { 3, "It's too late. Hurry the other way and find someone who will help you!", 0xFFFFFF57 },
    { 3, "You can find help if you can get past them. You have to be strong!", 0xFFFFFF57 },
    { 3, "He hates you.", 0xFFFFFF57 },
    { 3, "You have to get up.", 0xFFFFFF57 },
    { 3, "You can get out this time, but you have to hurry.", 0xFFFFFF57 },
    { 3, "Please let me out.", 0xFFFFFFFF },
    { 3, "PLEASE!", 0xFFFFFFFF },
    { 3, "...please let me out....", 0xFFFFFFFF },
    // scene 5 — the birthday finale (blue friend, gray bullies, white child)
    { 5, "Wow, your brother is kind of a baby isn't he?", 0x57C0FF },
    { 5, "It's hilarious.", 0x787878 },
    { 5, "Why don't we help him get a closer look! He will love it!", 0x787878 },
    { 5, "No! Please!", 0xFFFFFFFF },
    { 5, "Come on guys, let's give this little man a lift. He wants to get a closer look!", 0x787878 },
    { 5, "No! I don't want to go!", 0xFFFFFFFF },
    { 5, "You heard the little man! He wants to get even closer! Ha ha ha!", 0x787878 },
    { 5, "Hey guys, I think the little man said he wants to give Fredbear a big kiss!", 0x787878 },
    { 5, "On THREE! One.... two.....", 0x787878 },
    { 5, "These are my friends.", 0xFFFFFFFF },
};

} // file-local

// the renderer's read-only access to the recovered script
const char* Fnaf4CsLineText(i32 row) {
    const i32 n = (i32)(sizeof(kCsScript) / sizeof(kCsScript[0]));
    return (row >= 0 && row < n) ? kCsScript[row].text : "";
}
u32 Fnaf4CsLineColor(i32 row) {
    const i32 n = (i32)(sizeof(kCsScript) / sizeof(kCsScript[0]));
    return (row >= 0 && row < n) ? kCsScript[row].color : 0xFFFFFFFF;
}

void FNaF4Game::TickCutscene(f32 dt, const FNaF4Inputs& in) {
    CutsceneState& cs = m_cut;
    // the walk (g11/12: 100 ms, 25 px) in the house pages; the beds and
    // walls bound (dump frame 12 layout: houseHal b/walls at the edges)
    cs.stepT += dt;
    if (!cs.done && cs.stepT >= 0.1f) {
        cs.stepT -= 0.1f;
        f32 nx = cs.px, ny = cs.py;
        if (in.rightPressed) nx += 25.0f;
        if (in.leftPressed)  nx -= 25.0f;
        if (in.downPressed)  ny += 25.0f;
        if (in.upPressed)    ny -= 25.0f;
        if (nx < 180.0f) nx = 180.0f;
        if (nx > 1900.0f) nx = 1900.0f;
        if (ny < 200.0f) ny = 200.0f;
        if (ny > 560.0f) ny = 560.0f;
        cs.px = nx; cs.py = ny;
    }
    // the screen follow (g14-19: the 384/512 mid-lines, 768/1024 pages)
    if (cs.px > cs.camX + 512.0f + 512.0f) cs.camX += 1024.0f;
    if (cs.px < cs.camX + 512.0f - 512.0f) cs.camX -= 1024.0f;
    if (cs.py > cs.camY + 384.0f + 384.0f) cs.camY += 768.0f;
    if (cs.py < cs.camY + 384.0f - 384.0f) cs.camY -= 768.0f;
    if (cs.camX < 0.0f) cs.camX = 0.0f;
    if (cs.camY < 0.0f) cs.camY = 0.0f;
    if (cs.camX > 4096.0f) cs.camX = 4096.0f;
    if (cs.camY > 3072.0f) cs.camY = 3072.0f;

    // the scene timer + the REAL dialogue playback (the dump's act #88
    // lines, one per beat; Space/A advances like the original's click)
    cs.textT += dt;
    {
        // the lines of this scene
        i32 first = -1, count = 0;
        for (i32 i = 0; i < (i32)(sizeof(kCsScript) / sizeof(kCsScript[0])); ++i) {
            if (kCsScript[i].scene != cs.scene) continue;
            if (first < 0) first = i;
            count += 1;
        }
        if (first >= 0) {
            const f32 kLineBeat = 3.0f;
            cs.lineT += dt;
            if (cs.lineT >= kLineBeat || in.aPressed) {
                cs.lineT = 0.0f;
                if (cs.line < count - 1) cs.line += 1;
            }
            cs.curLine = first + cs.line;
            cs.lineCount = count;
        }
    }
    if (in.bPressed || cs.textT >= 90.0f) cs.done = true;
    if (cs.done) {
        cs.doneT += dt;
        if (cs.doneT >= 1.0f) {
            cs.doneT = 0.0f;
            SfxStop("snd_Deep_Ambience_With_Sc");
            if (cs.scene <= 0) {
                GoWhatNight(1);                        // g189: scene 0 -> card
            } else if (cs.scene <= 4) {
                m_minigamePlay = false;                // g190-193: V5
                m_pt.game = 0;
                m_screen = SCR_INTRO;                  // -> intro to plushtrap
                m_cardT = 0.0f;
            } else {
                m_screen = SCR_TITLE;                  // g194: -> title
            }
        }
    }
}

// ---- the ending (frame 13): the typewriter talk box; the dump's letters
// are sprite images (no string table) — a labeled stop-gap timeline ----

void FNaF4Game::TickEnding(f32 dt, const FNaF4Inputs& in) {
    m_endT += dt;
    m_endLetterT += dt;
    if (m_endLetterT >= 0.05f && m_endLine < 8) {      // the letter cadence
        m_endLetterT = 0.0f;
    }
    if (in.aPressed) m_endLine += 1;                   // Space/Enter advance
    if (m_endT >= 90.0f || in.bPressed || m_endLine >= 8) {
        SfxStop("snd_Deep_Ambience_With_Sc");
        m_screen = SCR_TITLE;
    }
}

// ------------------------------------------------------------
// bedroom
// ------------------------------------------------------------

void FNaF4Game::TickBedroom(f32 dt, const FNaF4Inputs& in) {
    if (m_gameover) return;

    // ---- walking (collapsed to the 5-position graph) ----
    if (m_walkT > 0.0f) {
        m_walkT -= dt;
        if (m_walkT <= 0.0f) { m_walkT = 0.0f; m_pos = m_walkTo; }
    } else {
        bool walk = false;
        Pos to = m_pos;
        i32 dir = 0;
        if (m_foxyGotYou) {
            // the forced flashlight: walking away or sleeping is fatal
            // (g44-46/257)
            if (in.leftPressed || in.rightPressed || in.upPressed || in.bPressed)
                TriggerJumpscare(642);
        } else if (in.leftPressed) {
            if (m_pos == P_CENTER)      { to = P_LEFT;   dir = 45;  walk = true; }
            else if (m_pos == P_CLOSET) { to = P_CENTER; dir = 45;  walk = true; }
            else if (m_pos == P_RIGHT)  { to = P_CLOSET; dir = 45;  walk = true; }
        } else if (in.rightPressed) {
            if (m_pos == P_CENTER)      { to = P_CLOSET; dir = 160; walk = true; }
            else if (m_pos == P_CLOSET) { to = P_RIGHT;  dir = 160; walk = true; }
            else if (m_pos == P_LEFT)   { to = P_CENTER; dir = 160; walk = true; }
        } else if (in.upPressed && m_pos == P_CENTER) {
            to = P_BED; dir = 57; walk = true;          // to the bed
        } else if ((in.downPressed || in.bPressed) && m_pos == P_BED) {
            to = P_CENTER; dir = 57; walk = true;       // out of the bed
        }
        if (walk) {
            m_walkTo = to;
            m_walkDir = dir;
            m_walkT = 0.45f;
            m_peeking = false;
            Sfx("snd_carpetrun1b", false, 5, 100);
        }
    }

    // ---- doors (X hold at the doors; creaks per g79-86/193-196) ----
    m_prevLeftShut = m_leftShut;  m_prevRightShut = m_rightShut;
    m_leftShut  = (m_pos == P_LEFT  && in.xHeld && m_walkT <= 0.0f);
    m_rightShut = (m_pos == P_RIGHT && in.xHeld && m_walkT <= 0.0f);
    if (m_leftShut && !m_prevLeftShut)
        Sfx((m_miniArt % 2) ? "snd_doorcreak1" : "snd_doorcreak3", false, 7, 100);
    if (!m_leftShut && m_prevLeftShut)
        Sfx((m_miniArt % 2) ? "snd_door2a" : "snd_door2b", false, 7, 100);
    if (m_rightShut && !m_prevRightShut)
        Sfx((m_miniArt % 2) ? "snd_doorcreak2" : "snd_doorcreak4", false, 7, 100);
    if (!m_rightShut && m_prevRightShut)
        Sfx("snd_door2c", false, 7, 100);

    // ---- the flashlight peek (A hold; the click per g129-132) ----
    const bool wasPeeking = m_peeking;
    m_peeking = (in.aHeld && m_walkT <= 0.0f &&
                 (m_pos == P_LEFT || m_pos == P_RIGHT || m_pos == P_CLOSET || m_pos == P_BED));
    if (m_peeking && !wasPeeking)
        Sfx("snd_FLASHLIG_7_GEN-HDF11244", false, 10, 100);

    // ---- listening (standing at a door holding nothing; g185-192) ----
    m_listening = 0;
    if (m_walkT <= 0.0f && !m_peeking && !in.xHeld) {
        if (m_pos == P_LEFT)  m_listening = 1;
        if (m_pos == P_RIGHT) m_listening = 2;
    }
    ChVol(20, (m_listening == 1 && m_bonniePos == 1) ||
              (m_listening == 2 && m_chicaPos  == 1) ? 100 : 0);
    // Chica's kitchen (g306-308): audible through the right-door listen
    ChVol(19, m_listening == 2 ? (m_chicaPos == 0 ? 100 : 40) : 0);

    // ---- the lanes ----
    TickLanes(dt);
    TickFoxy(dt, m_peeking && m_pos == P_CLOSET);
    TickFreddy(dt, m_peeking && m_pos == P_BED);
    TickParanoia(dt);

    // Fredbear's closet breathing while watched (g97/98)
    if (m_fredbearAI > 0 && m_foxyIn)
        ChVol(27, (m_peeking && m_pos == P_CLOSET) ? 100 : 0);
    else
        ChVol(27, 0);

    // ---- the door-flash attacks (g201/202; DEVIATION: a shut door
    // blocks the flash-scare — the peek shows the closed door) ----
    if (m_peeking && m_pos == P_LEFT  && m_bonniePos == 1 && !m_leftShut)
        TriggerJumpscare(487);
    if (m_peeking && m_pos == P_RIGHT && m_chicaPos  == 1 && !m_rightShut)
        TriggerJumpscare(450);

    // ---- the bedroom attacks (g216/217: lie down while they linger) ----
    if (m_pos == P_BED && m_walkT <= 0.0f) {
        if (m_bonnieBed) { m_bonnieBed = false; TriggerJumpscare(592); }
        else if (m_chicaBed) { m_chicaBed = false; TriggerJumpscare(358); }
    }

    TickClock(dt);
}

// ------------------------------------------------------------
// clock (g380-385): raw ms -> 60 s per hour
// ------------------------------------------------------------

void FNaF4Game::TickClock(f32 dt) {
    m_hourClock += dt;
    // the "fast nights" cheat (extras): shorter hours — the factor is a
    // labeled approximation (the office's exact use isn't traced)
    const f32 kHour = m_fastNights ? 30.0f : 60.0f;
    if (m_hourClock >= kHour) {
        m_hourClock -= kHour;
        m_hour += 1;
        Sfx("snd_clockchime7", false, 3, 60);
        ApplyNightAI();
        if (m_hour >= 6) {                      // g385 -> the 5->6 AM screen
            m_screen = SCR_NIGHTWIN;
            m_cardT = 0.0f;
            Sfx("snd_alarmclock4", false, 2, 100);
        }
    }
}

// ------------------------------------------------------------
// Bonnie/Chica lanes (g150-167, 193-217, 298-308, 396/397)
// ------------------------------------------------------------

void FNaF4Game::TickLanes(f32 dt) {
    // the shared 5 s opportunity roll (g150/151)
    m_laneRollT += dt;
    if (m_laneRollT >= 5.0f) {
        m_laneRollT = 0.0f;
        if (m_listening != 1 && !m_leftShut && (rand() % 20) + 1 <= m_bonnieAI && m_bonniePos == 0) {
            m_bonniePos = 1;
            Sfx((m_miniArt % 2) ? "snd_walk1b" : "snd_walk2b", false, 5, 100);
        }
        if (m_listening != 2 && !m_rightShut && (rand() % 20) + 1 <= m_chicaAI && m_chicaPos == 0) {
            m_chicaPos = 1;
            Sfx((m_miniArt % 2) ? "snd_walk3b" : "snd_walk4b", false, 5, 100);
        }
    }

    // ignored at the door: he retreats after 10 s (g161/167)
    if (m_bonniePos == 1) {
        m_bonnieIgnoreT += dt;
        if (m_bonnieIgnoreT >= 10.0f && m_listening != 1 && !m_leftShut) {
            m_bonniePos = 0;
            m_bonnieIgnoreT = 0.0f;
            m_bonnieNearT = 0.0f;
            Sfx("snd_walk2b", false, 5, 100);
        }
    } else m_bonnieIgnoreT = 0.0f;
    if (m_chicaPos == 1) {
        m_chicaIgnoreT += dt;
        if (m_chicaIgnoreT >= 10.0f && m_listening != 2 && !m_rightShut) {
            m_chicaPos = 0;
            m_chicaIgnoreT = 0.0f;
            m_chicaNearT = 0.0f;
            Sfx("snd_walk4b", false, 5, 100);
        }
    } else m_chicaIgnoreT = 0.0f;

    // the shut-door visit: he comes, then leaves after 3 s (g197-200)
    if (m_leftShut && m_bonniePos == 1) {
        m_bonnieVisitT += dt;
        if (m_bonnieVisitT >= 3.0f) {
            m_bonnieVisitT = 0.0f;
            m_bonniePos = 0;
            Sfx("snd_walk1b", false, 5, 100);
        }
    } else m_bonnieVisitT = 0.0f;
    if (m_rightShut && m_chicaPos == 1) {
        m_chicaVisitT += dt;
        if (m_chicaVisitT >= 3.0f) {
            m_chicaVisitT = 0.0f;
            m_chicaPos = 0;
            Sfx("snd_walk3b", false, 5, 100);
        }
    } else m_chicaVisitT = 0.0f;

    // the linger timers: +1 per second at near, spent by lying down
    // (g298/303, alterable[6] > 20 + Night*4)
    const i32 kLinger = 20 + m_night * 4;
    if (m_bonniePos == 1) {
        m_bonnieNearT += dt;
        if (m_peeking && m_pos == P_LEFT) m_bonnieNearT = 0.0f;   // g303
        if (m_bonnieNearT > (f32)kLinger) m_bonnieBed = true;
    }
    if (m_chicaPos == 1) {
        m_chicaNearT += dt;
        if (m_peeking && m_pos == P_RIGHT) m_chicaNearT = 0.0f;   // g298-300
        if (m_chicaNearT > (f32)kLinger) m_chicaBed = true;
    }
}

// ------------------------------------------------------------
// the closet lane (g92-148, 255-258; Fredbear per g97/111/433/444)
// ------------------------------------------------------------

void FNaF4Game::TickFoxy(f32 dt, bool viewingCloset) {
    if (m_gameover) return;

    // the entry roll (g119): every 5 s, Random(10)+1 <= AI; Fredbear takes
    // the lane when his AI is up (g111/444)
    if (!m_foxyIn) {
        const i32 ai = m_fredbearAI > 0 ? m_fredbearAI : m_foxyAI;
        if (ai > 0) {
            m_foxyRollT += dt;
            if (m_foxyRollT >= 5.0f) {
                m_foxyRollT = 0.0f;
                if ((rand() % 10) + 1 <= ai) {
                    m_foxyIn = true;
                    m_closetCounter = 3 + rand() % 5;      // g146
                    Sfx("snd_foxyrun1", false, 5, 100);
                    Sfx("snd_runandenter", false, 5, 100);
                }
            }
        }
        return;
    }

    // the stage counter: -1/s watched, +1/s unwatched (g136-140), 0..10
    m_closetTickT += dt;
    if (m_closetTickT >= 1.0f) {
        m_closetTickT = 0.0f;
        if (viewingCloset) {
            if (m_closetCounter > 0) m_closetCounter -= 1;
        } else {
            if (m_closetCounter < 10) m_closetCounter += 1;
        }
        if (m_closetCounter >= 10) m_foxyGotYou = 1;   // g147
        if (m_closetCounter < 10)  m_foxyGotYou = 0;   // g148
    }

    // the bite: flashing while he is at stage 3 (g255) — non-fatal, but it
    // does not stop the drain; Fredbear's version kills (g371)
    if (viewingCloset && m_closetCounter >= 6 && m_fredbearSndT <= 0.0f) {
        m_fredbearSndT = 3.0f;
        if (m_fredbearAI > 0) {
            TriggerJumpscare(358);                     // the fredbear scare
        } else {
            Sfx("snd_screamshort", false, 16, 100);
            m_biteT = 1.2f;                            // the bite overlay
        }
    }
    if (m_fredbearSndT > 0.0f) m_fredbearSndT -= dt;
}

// ------------------------------------------------------------
// the bed lane (g227-254, 399, 433)
// ------------------------------------------------------------

void FNaF4Game::TickFreddy(f32 dt, bool viewingBed) {
    if (m_gameover || m_fredbearAI > 0) return;

    m_freddyT += dt;
    if (m_freddyT >= 4.0f) {          // g227: timer 4000, not viewing bed
        m_freddyT = 0.0f;
        if (!viewingBed) m_freddyCounter += m_freddyAI;
    }
    if (viewingBed) {                 // g231: drains 1/s while you look
        m_bedTickT += dt;
        if (m_bedTickT >= 1.0f) {
            m_bedTickT = 0.0f;
            if (m_freddyCounter > 0) m_freddyCounter -= 1;
        }
    }

    // the minimonsters loop (g247/248/251/252)
    if (viewingBed && m_freddyCounter >= 10)      ChVol(21, 50);
    else if (!viewingBed && m_freddyCounter >= 30) ChVol(21, 2);
    else if (!viewingBed && m_freddyCounter >= 20) ChVol(21, 1);
    else                                          ChVol(21, 0);

    // the bed attack (g253/254): counter >= 60 while in bed
    if (m_freddyCounter >= 60 && m_pos == P_BED && m_walkT <= 0.0f)
        TriggerJumpscare(620);

    // the hallucination wall (g283-288): counter >= 80 — DEVIATION: the
    // dump's delayed forced flash collapsed to an immediate attack
    if (m_freddyCounter >= 80)
        TriggerJumpscare(m_fredbearAI > 0 ? 358 : 368);
}

// ------------------------------------------------------------
// paranoia (g259-288)
// ------------------------------------------------------------

void FNaF4Game::TickParanoia(f32 dt) {
    m_dangerT += dt;
    if (m_dangerT >= 4.0f) {          // g275: every 4 s
        m_dangerT = 0.0f;
        i32 total = 0;
        total += (m_bonniePos == 1) ? 2 : (m_bonniePos == 0 ? 1 : 0);
        total += (m_chicaPos  == 1) ? 2 : (m_chicaPos  == 0 ? 1 : 0);
        if (m_foxyIn) total += 2;
        if (m_freddyCounter < 20)      total += 0;
        else if (m_freddyCounter < 30) total += 1;
        else if (m_freddyCounter < 50) total += 3;
        else                           total += 5;
        m_totalDanger = total;
        // the flash schedule (g276-279): 5/4/3/2 s by danger
        if (total >= 5)      m_bfTimer = 2.0f;
        else if (total == 4) m_bfTimer = 3.0f;
        else if (total == 3) m_bfTimer = 4.0f;
        else if (total == 2) m_bfTimer = 5.0f;
    }

    if (m_totalDanger >= 2) {
        m_bfTimer -= dt;
        if (m_bfTimer <= 0.0f && m_bfHold <= 0.0f) {
            // the show length (g276-279): (5+R5)/(10+R10)/(10+R10)/(10+R30)
            i32 frames;
            if      (m_totalDanger >= 5) frames = 10 + rand() % 30;
            else if (m_totalDanger == 4) frames = 10 + rand() % 10;
            else if (m_totalDanger == 3) frames = 10 + rand() % 10;
            else                         frames = 5  + rand() % 5;
            m_bfHold = (f32)frames / 60.0f;
        }
    }
    if (m_bfHold > 0.0f) {
        m_bfHold -= dt;
        m_blackFlashA = (m_bfHold > 0.1f) ? 255.0f
                       : (m_bfHold / 0.1f) * 255.0f;   // fast fade
    } else {
        m_blackFlashA = 0.0f;
    }
}

// ---- v2.64: the fn4 save bridge ----

void FNaF4Game::ApplyProgressF4(const Progress::GameProgressF4& p) {
    m_lastNight = p.night < 1 ? 1 : (p.night > 6 ? 6 : p.night);
    m_night = p.night < 1 ? 1 : (p.night > 8 ? 8 : p.night);
    m_scene = p.scene;
    m_beat5 = p.beat5;
    m_beat6 = p.beat6;
    m_beat7 = p.beat7;
    m_beat8 = p.beat8;
    m_s1 = p.s1; m_s2 = p.s2; m_s3 = p.s3;
    m_s4 = p.s4; m_s5 = p.s5; m_s6 = p.s6;
    m_testFlag = p.test;
    m_cheatHouseMap = p.cheatHouseMap;
    m_fastNights = p.fastNights;
    m_cheatRadar = p.cheatRadar;
    m_blindMode = p.blindMode;
    m_instaFoxy = p.instaFoxy;
    m_madFreddy = p.madFreddy;
    m_allNightmare = p.allNightmare;
}

void FNaF4Game::FillProgressF4(Progress::GameProgressF4& p) const {
    p.night = m_night;
    p.scene = m_scene;
    p.beat5 = m_beat5;
    p.beat6 = m_beat6;
    p.beat7 = m_beat7;
    p.beat8 = m_beat8;
    p.s1 = m_s1; p.s2 = m_s2; p.s3 = m_s3;
    p.s4 = m_s4; p.s5 = m_s5; p.s6 = m_s6;
    p.test = m_testFlag;
    p.cheatHouseMap = m_cheatHouseMap;
    p.fastNights = m_fastNights;
    p.cheatRadar = m_cheatRadar;
    p.blindMode = m_blindMode;
    p.instaFoxy = m_instaFoxy;
    p.madFreddy = m_madFreddy;
    p.allNightmare = m_allNightmare;
}

} // namespace fnaf
