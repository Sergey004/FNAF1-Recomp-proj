/**
 * Five Nights at Freddy's 1 — Recompilation
 * AudioSystem.h: XAudio2 sound playback for fnaf1.pak PCM sounds
 *
 * Xbox 360: XAudio2 (XDK) — up to 16 concurrent source voices.
 * Sounds are raw 16-bit PCM blobs loaded by PakLoader ("snd_*" entries).
 *
 * Wire-up is data-driven: every sound listed below is referenced by the
 * original game's event program (ctfak-cpp "Events Listing" dump):
 *   voiceover1c..5  — Phone Guy calls, one per night (frame 3 groups 361-365)
 *   XSCREAM         — jump scare scream (groups 228/322/408)
 *   circus          — power-out music box loop (group 269)
 *   powerdown       — power depletion (group 285)
 *   blip3           — menu navigation (title groups 28-31)
 *   static2 / darkness music — title ambience loops (title group 2)
 *   ... etc (see docs/AUDIO.md)
 */

#ifndef FNAF_AUDIO_SYSTEM_H
#define FNAF_AUDIO_SYSTEM_H

#include "Types.h"
#include <math.h>

namespace fnaf {

class PakLoader;

// Canonical pak sound names (as written by ctfak-cpp "Recomp Pack")
namespace Snd {
    // Menu / title
    static const char* const STATIC2        = "snd_static2";
    static const char* const DARKNESS_MUSIC = "snd_darkness music";
    static const char* const BLIP           = "snd_blip3";
    static const char* const PUT_DOWN       = "snd_put down";
    static const char* const DOOR_ERROR     = "snd_error";
    // Office ambience (frame 3 group 14)
    static const char* const COLD_PRESC     = "snd_ColdPresc B";
    static const char* const BUZZ_FAN       = "snd_Buzz_Fan_Florescent2";
    static const char* const BALLAST_HUM    = "snd_BallastHumMedium2";
    static const char* const ROBOT_VOICE    = "snd_robotvoice";
    // Phone calls (frame 3 groups 361-365)
    static const char* const VOICEOVER[5] = {
        "snd_voiceover1c", "snd_voiceover2a", "snd_voiceover3",
        "snd_voiceover4", "snd_voiceover5"
    };
    // Doors / buttons
    static const char* const DOOR_SLAM      = "snd_SFXBible_12478";
    static const char* const KNOCK          = "snd_knock2";
    static const char* const DOOR_POUNDING  = "snd_DOOR_POUNDING_ME_D0291401";
    static const char* const OVEN_DRAW[4] = {
        "snd_OVEN-DRA_1_GEN-HDF18119", "snd_OVEN-DRA_2_GEN-HDF18120",
        "snd_OVEN-DRA_7_GEN-HDF18121", "snd_OVEN-DRAWE_GEN-HDF18122"
    };
    // Cameras
    static const char* const CAMERA_SWITCH  = "snd_CAMERA_VIDEO_LOA_60105303";
    static const char* const TAPE_EJECT     = "snd_MiniDV_Tape_Eject_1";
    static const char* const STATIC_LOOP    = "snd_static";
    // Animatronics
    static const char* const DEEP_STEPS     = "snd_deep steps";
    static const char* const RUN            = "snd_run";
    static const char* const RUNNING_FAST   = "snd_running fast3";
    static const char* const FREDDY_LAUGH[3] = {
        "snd_Laugh_Giggle_Girl_1d", "snd_Laugh_Giggle_Girl_2d", "snd_Laugh_Giggle_Girl_8d"
    };
    static const char* const FREDDY_LAUGH_LONG = "snd_Laugh_Giggle_Girl_1";
    static const char* const PIRATE_SONG    = "snd_pirate song2";
    static const char* const WHISPERING     = "snd_whispering2";
    static const char* const WINDOW_SCARE   = "snd_windowscare";
    static const char* const GARBLE[3] = {
        "snd_garble1", "snd_garble2", "snd_garble3"
    };
    static const char* const BREATHS[4] = {
        "snd_Vocals_Breaths_S_35972006", "snd_Vocals_Breaths_S_35972008",
        "snd_Vocals_Breaths_S_35972012", "snd_Vocals_Breaths_S_35972014"
    };
    // Power out sequence (groups 285/269/271)
    static const char* const POWERDOWN      = "snd_powerdown";
    static const char* const AMBIENCE2      = "snd_ambience2";
    static const char* const CIRCUS         = "snd_circus";
    static const char* const MUSIC_BOX      = "snd_music box";
    // Jump scare / game over
    static const char* const XSCREAM        = "snd_XSCREAM";
    static const char* const XSCREAM2       = "snd_XSCREAM2";
    // 6 AM (night complete)
    static const char* const CHIMES         = "snd_chimes 2";
    static const char* const CROWD_KIDS     = "snd_CROWD_SMALL_CHIL_EC049202";
    // Misc
    static const char* const EERIE_AMBIENCE = "snd_EerieAmbienceLargeSca_MV005";
    static const char* const PARTY_FAVOR    = "snd_PartyFavorraspyPart_AC01__3";
    static const char* const COMPUTER_DIG   = "snd_COMPUTER_DIGITAL_L2076505";
}

// ------------------------------------------------------------
// v2.19: dB-based volume layer (Clickteam / DirectSound semantics).
//
// The original Clickteam "Set channel volume" values are 0..100 — a LINEAR
// amplitude percentage. Clickteam hands that to DirectSound where it is
// expressed as attenuation in dB (0 = unity, negative = quieter, ~-100 dB =
// silence). XAudio2 takes an amplitude multiplier (1.0 = unity), so we go
//   Clickteam 0..100  ->  dB  ->  XAudio2 amplitude
// via these two helpers. Keeping the mixer in dB (not amplitude) also lets a
// later positional-attenuation / master chain ADD gains instead of fudging
// multipliers.
// ------------------------------------------------------------

// Decibels -> amplitude multiplier (XAudio2 SetVolume convention).
//   0 dB = 1.0, -6 dB ~= 0.5, -20 dB = 0.1, -100 dB ~= silence.
// Uses the C89 double math functions to stay VS2010-safe (no C99 log10f/powf).
static inline float DbToAmplitude(float db) {
    return (float)pow(10.0, (double)db * 0.05);          // 10^(dB/20)
}

// Clickteam volume (0..100) -> dB. 100 = 0 dB (unity); 0 -> silence.
static inline float CFVolumeToDb(int cfVolume) {
    if (cfVolume <= 0) return -100.0f;       // Clickteam 0 = silence
    return (float)(20.0 * log10((double)cfVolume / 100.0));
}

// Linear amplitude (0..1) -> dB (inverse of DbToAmplitude).
static inline float AmplitudeToDb(float amp) {
    if (amp <= 0.0f) return -100.0f;
    return (float)(20.0 * log10((double)amp));
}

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    bool Init();
    void Shutdown();

    // Must be called once per frame to recycle finished voices
    void Tick();

    // Play a pak sound. loop=true repeats until Stop(). volume is a LINEAR
    // amplitude 0.0..1.0 (one-shot "juice" loudness; the mixer channels use dB).
    // Returns false if sound not found or no free voice.
    bool Play(PakLoader* pak, const char* sndName, bool loop, float volume);

    // v2.16 channel mixer: matches the original "Speaker" channel model.
    // Sounds are bound to a channel; the channel's volume is a live value
    // that events change (proximity, camera, doors). Returns false on failure.
    bool PlayOnChannel(PakLoader* pak, const char* sndName, bool loop, int channel);

    // Set a channel's volume in DECIBELS (v2.19): 0 dB = unity, negative =
    // quieter, ~-100 dB = silence. Use CFVolumeToDb() for original Clickteam
    // 0..100 values. Re-applied live to any voice on that channel (0..31).
    void SetChannelVolume(int channel, float volumeDb);
    float GetChannelVolume(int channel) const;   // returns dB

    // Stop every voice currently playing the given sound
    void Stop(const char* sndName);

    // Stop everything (used on state transitions)
    void StopAll();

    bool IsPlaying(const char* sndName) const;

private:
    struct VoiceSlot {
        void*  voice;        // IXAudio2SourceVoice*
        char   name[64];
        int    channel;      // v2.16: mixer channel this voice is bound to (-1 = none)
        bool   inUse;
    };

    void FreeSlot(int idx);
    bool PlayInternal(PakLoader* pak, const char* sndName, bool loop, float volume, int channel);

    void* m_xaudio;          // IXAudio2*
    void* m_master;          // IXAudio2MasteringVoice*
    VoiceSlot m_slots[16];
    int   m_slotCount;
    float m_channelVolumeDb[32];   // v2.19: per-channel volume in dB (default 0.0 = unity)
    bool  m_ok;
};

} // namespace fnaf

#endif // FNAF_AUDIO_SYSTEM_H
