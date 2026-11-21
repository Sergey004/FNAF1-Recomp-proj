/**
 * Five Nights at Freddy's 1 — Recompilation
 * AudioSystem.cpp: XAudio2 playback of pak sounds
 *
 * Ordinary sounds arrive normalized from PakLoader (RIFF headers peeled,
 * byte order fixed, 8-bit expanded to 16-bit). A compressed fmt=2 blob is
 * decoded from memory by AudioDecoders.cpp and its PCM16 allocation stays
 * alive for exactly as long as the source voice uses it.
 */

#include "AudioSystem.h"
#include "PakLoader.h"
#include <cstring>
#include <cstdio>

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#endif
#include <xaudio2.h>

// The Xbox 360 XDK names the end-of-stream buffer flag with an extra
// underscore (XAUDIO2_END_OF_STREAM, xaudio2.h line 117); newer PC SDK
// headers call it XAUDIO2_END_OFSTREAM. Normalize to the PC name so this
// file compiles unchanged against both toolsets.
#if !defined(XAUDIO2_END_OFSTREAM)
#define XAUDIO2_END_OFSTREAM XAUDIO2_END_OF_STREAM
#endif

namespace fnaf {

static const int MAX_VOICES = 16;

// WAVE_FORMAT_PCM, matching what RecompPack wrote into the pak
static const u16 PCM_FORMAT_TAG = 0x0001;
static const u32 PCM_BITS_PER_SAMPLE = 16;

static void ApplyOutputPan(IXAudio2SourceVoice* voice,
                           IXAudio2MasteringVoice* master,
                           u32 sourceChannels, float pan) {
    if (pan < -1.0f) pan = -1.0f;
    if (pan >  1.0f) pan =  1.0f;
    const float left  = (pan < 0.0f) ? 1.0f : 1.0f - pan;
    const float right = (pan > 0.0f) ? 1.0f : 1.0f + pan;
    float matrix[4] = { left, right, 0.0f, 0.0f };
    if (sourceChannels == 2) {
        matrix[0] = left;  matrix[1] = 0.0f;
        matrix[2] = 0.0f;  matrix[3] = right;
    }
    voice->SetOutputMatrix(reinterpret_cast<IXAudio2Voice*>(master),
                           sourceChannels, 2, matrix);
}

static PakLoadedSound* FindSoundCompat(PakLoader* pak, const char* name) {
    PakLoadedSound* snd = pak->FindSound(name);
    if (snd || strncmp(name, "snd_", 4) == 0) return snd;
    // Recomp Pack canonicalizes every entry as snd_<source basename>. Several
    // modules pass the event/sample name verbatim instead. Do not truncate:
    // a truncated alias could select a different long sound name.
    if (strlen(name) > 59) return 0;
    char full[64];
    memcpy(full, "snd_", 4);
    strcpy(full + 4, name);
    return pak->FindSound(full);
}

static bool SoundNameMatches(const char* canonical, const char* requested) {
    if (strcmp(canonical, requested) == 0) return true;
    return strncmp(canonical, "snd_", 4) == 0 &&
           strncmp(requested, "snd_", 4) != 0 &&
           strcmp(canonical + 4, requested) == 0;
}

AudioSystem::AudioSystem()
    : m_xaudio(0)
    , m_master(0)
    , m_slotCount(MAX_VOICES)
    , m_ok(false)
{
    for (int i = 0; i < 32; ++i) m_channelVolumeDb[i] = 0.0f;   // v2.19: unity (0 dB)
    for (int i = 0; i < 32; ++i) m_channelPan[i] = 0.0f;        // v2.91: centre
    for (int i = 0; i < MAX_VOICES; ++i) {
        m_slots[i].voice = 0;
        m_slots[i].name[0] = '\0';
        m_slots[i].channel = -1;
        m_slots[i].pan = 0.0f;
        m_slots[i].sourceChannels = 1;
        ResetDecodedAudio(m_slots[i].decoded);
        m_slots[i].inUse = false;
    }
}

AudioSystem::~AudioSystem() {
    Shutdown();
}

bool AudioSystem::Init() {
    if (m_ok) return true;
    HRESULT hr = XAudio2Create(reinterpret_cast<IXAudio2**>(&m_xaudio), 0, XAUDIO2_DEFAULT_PROCESSOR);
    if (FAILED(hr) || !m_xaudio) {
        printf("AudioSystem: XAudio2Create failed (0x%08X)\n", hr);
        m_xaudio = 0;
        return false;
    }
    hr = reinterpret_cast<IXAudio2*>(m_xaudio)->CreateMasteringVoice(
        reinterpret_cast<IXAudio2MasteringVoice**>(&m_master));
    if (FAILED(hr) || !m_master) {
        printf("AudioSystem: CreateMasteringVoice failed (0x%08X)\n", hr);
        reinterpret_cast<IXAudio2*>(m_xaudio)->Release();
        m_xaudio = 0;
        return false;
    }
    m_ok = true;
    printf("AudioSystem: XAudio2 ready\n");
    return true;
}

void AudioSystem::FreeSlot(int idx) {
    if (idx < 0 || idx >= m_slotCount) return;
    VoiceSlot& s = m_slots[idx];
    if (s.voice) {
        IXAudio2SourceVoice* v = reinterpret_cast<IXAudio2SourceVoice*>(s.voice);
        v->Stop(0);
        v->FlushSourceBuffers();
        v->DestroyVoice();
        s.voice = 0;
    }
    s.name[0] = '\0';
    s.channel = -1;
    s.pan = 0.0f;
    s.sourceChannels = 1;
    FreeDecodedAudio(s.decoded);
    s.inUse = false;
}

void AudioSystem::Shutdown() {
    if (!m_ok) return;
    for (int i = 0; i < m_slotCount; ++i) FreeSlot(i);
    if (m_master) {
        reinterpret_cast<IXAudio2MasteringVoice*>(m_master)->DestroyVoice();
        m_master = 0;
    }
    if (m_xaudio) {
        reinterpret_cast<IXAudio2*>(m_xaudio)->Release();
        m_xaudio = 0;
    }
    m_ok = false;
}

void AudioSystem::Tick() {
    if (!m_ok) return;
    for (int i = 0; i < m_slotCount; ++i) {
        VoiceSlot& s = m_slots[i];
        if (!s.inUse || !s.voice) continue;
        IXAudio2SourceVoice* v = reinterpret_cast<IXAudio2SourceVoice*>(s.voice);
        XAUDIO2_VOICE_STATE st;
        v->GetState(&st);
        if (st.BuffersQueued == 0) {
            // Non-looped sound finished — recycle the voice
            FreeSlot(i);
        }
    }
}

bool AudioSystem::PlayInternal(PakLoader* pak, const char* sndName, bool loop, float volume, int channel) {
    if (!m_ok || !pak || !sndName || !sndName[0]) return false;

    PakLoadedSound* snd = FindSoundCompat(pak, sndName);
    if (!snd) {
        printf("AudioSystem: sound not found: %s\n", sndName);
        return false;
    }
    if (!snd->data || snd->dataSize == 0) return false;

    const u8* audioData = snd->data;
    u32 audioSize = snd->dataSize;
    u32 audioSampleRate = snd->sampleRate ? snd->sampleRate : 22050;
    u32 audioChannels = snd->channels ? snd->channels : 1;
    DecodedAudio decoded;
    ResetDecodedAudio(decoded);

    // Normalized FNAF1/PCM blobs keep their existing zero-decode path.
    // A PCM entry rejected by normalization (OOM, unsupported bit depth)
    // must fail closed, not be misdetected as an arbitrary MPEG stream.
    if (snd->format != 0 && snd->pakFmt == 0) {
        printf("AudioSystem: %s is not playable PCM\n", sndName);
        return false;
    }

    // Passthrough blobs are decoded here and owned by the voice slot until
    // XAudio2 has stopped and destroyed the voice.
    if (snd->format != 0) {
        if (!DecodeAudioMemory(snd->data, snd->dataSize, decoded)) {
            printf("AudioSystem: cannot decode %s (pak fmt=%u)\n", sndName, snd->pakFmt);
            return false;
        }
        audioData = decoded.data;
        audioSize = decoded.dataSize;
        audioSampleRate = decoded.sampleRate;
        audioChannels = decoded.channels;
    }

    // Find a free slot (steal the oldest non-looping slot as fallback)
    int slot = -1;
    for (int i = 0; i < m_slotCount; ++i) {
        if (!m_slots[i].inUse) { slot = i; break; }
    }
    if (slot < 0) {
        for (int i = 0; i < m_slotCount; ++i) { slot = i; break; } // steal slot 0
        FreeSlot(slot);
    }

    WAVEFORMATEX wfx;
    memset(&wfx, 0, sizeof(wfx));
    wfx.wFormatTag      = PCM_FORMAT_TAG;
    wfx.nChannels       = (WORD)audioChannels;
    wfx.nSamplesPerSec  = audioSampleRate;
    wfx.wBitsPerSample  = (WORD)PCM_BITS_PER_SAMPLE;
    wfx.nBlockAlign     = (WORD)(wfx.nChannels * wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    IXAudio2SourceVoice* voice = 0;
    HRESULT hr = reinterpret_cast<IXAudio2*>(m_xaudio)->CreateSourceVoice(
        &voice, &wfx, 0, XAUDIO2_DEFAULT_FILTER_FREQUENCY);
    if (FAILED(hr) || !voice) {
        printf("AudioSystem: CreateSourceVoice failed for %s (0x%08X)\n", sndName, hr);
        FreeDecodedAudio(decoded);
        return false;
    }
    voice->SetVolume(volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume));

    XAUDIO2_BUFFER buf;
    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = audioSize;
    buf.pAudioData = audioData;
    buf.Flags      = XAUDIO2_END_OFSTREAM;
    buf.LoopCount  = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = voice->SubmitSourceBuffer(&buf);
    if (FAILED(hr)) {
        printf("AudioSystem: SubmitSourceBuffer failed for %s (0x%08X)\n", sndName, hr);
        voice->DestroyVoice();
        FreeDecodedAudio(decoded);
        return false;
    }
    hr = voice->Start(0);
    if (FAILED(hr)) {
        voice->DestroyVoice();
        FreeDecodedAudio(decoded);
        return false;
    }

    // v2.91: bind the channel's current pan to this voice through its
    // output matrix. Normalized PCM keeps the proven FNAF1 legacy mapping
    // (one source channel -> stereo). Only the new decoder path needs the
    // real source channel count for correct stereo MP3/WAV playback.
    const u32 matrixChannels = (snd->format != 0) ? audioChannels : 1;
    float pan = (channel >= 0 && channel < 32) ? m_channelPan[channel] : 0.0f;
    ApplyOutputPan(voice, reinterpret_cast<IXAudio2MasteringVoice*>(m_master),
                   matrixChannels, pan);

    VoiceSlot& s = m_slots[slot];
    s.voice = voice;
    s.inUse = true;
    s.channel = channel;
    s.pan = pan;
    s.sourceChannels = matrixChannels;
    s.decoded = decoded;             // transfer ownership to the live voice
    ResetDecodedAudio(decoded);
    strncpy(s.name, snd->name, sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    return true;
}

bool AudioSystem::Play(PakLoader* pak, const char* sndName, bool loop, float volume) {
    return PlayInternal(pak, sndName, loop, volume, -1);
}

bool AudioSystem::PlayOnChannel(PakLoader* pak, const char* sndName, bool loop, int channel) {
    // v2.90: Clickteam semantics — a CHANNEL plays ONE sound. The old voice
    // on this channel MUST die here: without this, every office re-entry
    // (11 loops per entry in FNAF2) STACKED another voice per channel and
    // SetChannelVolume then amplified them all — the "almost went deaf"
    // report. (Play() on channel -1 keeps its fire-and-forget semantics.)
    if (channel >= 0) {
        for (int i = 0; i < m_slotCount; ++i) {
            if (m_slots[i].inUse && m_slots[i].channel == channel)
                FreeSlot(i);
        }
    }
    // Play at the channel's CURRENT dB; later SetChannelVolume re-applies the
    // (possibly ramping) volume to this live voice.
    return PlayInternal(pak, sndName, loop, DbToAmplitude(GetChannelVolume(channel)), channel);
}

void AudioSystem::SetChannelVolume(int channel, float volumeDb) {
    if (channel < 0 || channel >= 32) return;
    if (volumeDb < -100.0f) volumeDb = -100.0f;   // silence floor
    if (volumeDb >    0.0f) volumeDb =    0.0f;   // unity ceiling (no gain boost)
    m_channelVolumeDb[channel] = volumeDb;
    const float amp = DbToAmplitude(volumeDb);
    // Re-apply to every live voice bound to this channel.
    for (int i = 0; i < m_slotCount; ++i) {
        if (m_slots[i].inUse && m_slots[i].channel == channel && m_slots[i].voice) {
            reinterpret_cast<IXAudio2SourceVoice*>(m_slots[i].voice)->SetVolume(amp);
        }
    }
}

float AudioSystem::GetChannelVolume(int channel) const {
    if (channel < 0 || channel >= 32) return 0.0f;
    return m_channelVolumeDb[channel];
}

void AudioSystem::SetChannelPan(int channel, float pan) {
    if (channel < 0 || channel >= 32) return;
    if (pan < -1.0f) pan = -1.0f;
    if (pan >  1.0f) pan =  1.0f;
    m_channelPan[channel] = pan;
    // Re-apply live to every voice bound to this channel.
    for (int i = 0; i < m_slotCount; ++i) {
        if (m_slots[i].inUse && m_slots[i].channel == channel && m_slots[i].voice) {
            ApplyOutputPan(reinterpret_cast<IXAudio2SourceVoice*>(m_slots[i].voice),
                           reinterpret_cast<IXAudio2MasteringVoice*>(m_master),
                           m_slots[i].sourceChannels, pan);
        }
    }
}

float AudioSystem::GetChannelPan(int channel) const {
    if (channel < 0 || channel >= 32) return 0.0f;
    return m_channelPan[channel];
}

void AudioSystem::Stop(const char* sndName) {
    if (!m_ok || !sndName) return;
    for (int i = 0; i < m_slotCount; ++i) {
        if (m_slots[i].inUse && SoundNameMatches(m_slots[i].name, sndName)) {
            FreeSlot(i);
        }
    }
}

void AudioSystem::StopAll() {
    for (int i = 0; i < m_slotCount; ++i) FreeSlot(i);
}

bool AudioSystem::IsPlaying(const char* sndName) const {
    if (!m_ok || !sndName) return false;
    for (int i = 0; i < m_slotCount; ++i) {
        if (m_slots[i].inUse && SoundNameMatches(m_slots[i].name, sndName)) return true;
    }
    return false;
}

} // namespace fnaf
