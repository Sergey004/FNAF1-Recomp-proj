/**
 * Five Nights at Freddy's 1 — Recompilation
 * AudioSystem.cpp: XAudio2 playback of fnaf1.pak PCM sounds
 *
 * Xbox 360 XDK ships XAudio2 2.x; the same code path also exists in the
 * PC DirectX SDK end-user runtime, guarded by the _XBOX define already used
 * across the project. v2.6: sounds arrive normalized from PakLoader
 * (RIFF headers peeled, byte order fixed, 8-bit expanded to 16-bit) —
 * playback needs only a WAVEFORMATEX + a source voice per stream.
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

AudioSystem::AudioSystem()
    : m_xaudio(0)
    , m_master(0)
    , m_slotCount(MAX_VOICES)
    , m_ok(false)
{
    for (int i = 0; i < 32; ++i) m_channelVolumeDb[i] = 0.0f;   // v2.19: unity (0 dB)
    for (int i = 0; i < MAX_VOICES; ++i) {
        m_slots[i].voice = 0;
        m_slots[i].name[0] = '\0';
        m_slots[i].channel = -1;
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

    PakLoadedSound* snd = pak->FindSound(sndName);
    if (!snd) {
        printf("AudioSystem: sound not found: %s\n", sndName);
        return false;
    }
    if (snd->format != 0) {
        // v2.6: PakLoader::NormalizeSounds() rewrites every entry to
        // format 0 (little-endian PCM16, RIFF headers peeled, byte order
        // fixed, 8-bit expanded). Anything left at format 1 is a codec
        // the 360 build cannot play.
        printf("AudioSystem: %s is not playable PCM (fmt=%u)\n", sndName, snd->format);
        return false;
    }
    if (!snd->data || snd->dataSize == 0) return false;

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
    wfx.nChannels       = (WORD)(snd->channels ? snd->channels : 1);
    wfx.nSamplesPerSec  = snd->sampleRate ? snd->sampleRate : 22050;
    wfx.wBitsPerSample  = (WORD)PCM_BITS_PER_SAMPLE;
    wfx.nBlockAlign     = (WORD)(wfx.nChannels * wfx.wBitsPerSample / 8);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

    IXAudio2SourceVoice* voice = 0;
    HRESULT hr = reinterpret_cast<IXAudio2*>(m_xaudio)->CreateSourceVoice(
        &voice, &wfx, 0, XAUDIO2_DEFAULT_FILTER_FREQUENCY);
    if (FAILED(hr) || !voice) {
        printf("AudioSystem: CreateSourceVoice failed for %s (0x%08X)\n", sndName, hr);
        return false;
    }
    voice->SetVolume(volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume));

    XAUDIO2_BUFFER buf;
    memset(&buf, 0, sizeof(buf));
    buf.AudioBytes = snd->dataSize;
    buf.pAudioData = snd->data;
    buf.Flags      = XAUDIO2_END_OFSTREAM;
    buf.LoopCount  = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = voice->SubmitSourceBuffer(&buf);
    if (FAILED(hr)) {
        printf("AudioSystem: SubmitSourceBuffer failed for %s (0x%08X)\n", sndName, hr);
        voice->DestroyVoice();
        return false;
    }
    hr = voice->Start(0);
    if (FAILED(hr)) {
        voice->DestroyVoice();
        return false;
    }

    VoiceSlot& s = m_slots[slot];
    s.voice = voice;
    s.inUse = true;
    s.channel = channel;
    strncpy(s.name, sndName, sizeof(s.name) - 1);
    s.name[sizeof(s.name) - 1] = '\0';
    return true;
}

bool AudioSystem::Play(PakLoader* pak, const char* sndName, bool loop, float volume) {
    return PlayInternal(pak, sndName, loop, volume, -1);
}

bool AudioSystem::PlayOnChannel(PakLoader* pak, const char* sndName, bool loop, int channel) {
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

void AudioSystem::Stop(const char* sndName) {
    if (!m_ok || !sndName) return;
    for (int i = 0; i < m_slotCount; ++i) {
        if (m_slots[i].inUse && strcmp(m_slots[i].name, sndName) == 0) {
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
        if (m_slots[i].inUse && strcmp(m_slots[i].name, sndName) == 0) return true;
    }
    return false;
}

} // namespace fnaf
