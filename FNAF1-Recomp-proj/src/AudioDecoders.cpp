#include "AudioDecoders.h"

#include <cstring>

// The game already has the compressed bytes in memory; do not pull stdio,
// wchar file APIs, or host SIMD assumptions into the Xbox 360 build.
#define DR_WAV_NO_STDIO
#define DR_WAV_NO_WCHAR
#define DR_WAV_IMPLEMENTATION
#include "third_party/dr_wav.h"

#define DR_MP3_NO_STDIO
#define DR_MP3_NO_SIMD
#define DR_MP3_IMPLEMENTATION
#include "third_party/dr_mp3.h"

namespace fnaf {

void ResetDecodedAudio(DecodedAudio& audio) {
    audio.data = 0;
    audio.dataSize = 0;
    audio.sampleRate = 0;
    audio.channels = 0;
    audio.allocation = 0;
    audio.owner = DECODED_AUDIO_NONE;
}

void FreeDecodedAudio(DecodedAudio& audio) {
    if (audio.allocation) {
        if (audio.owner == DECODED_AUDIO_DR_WAV) {
            drwav_free(audio.allocation, NULL);
        } else if (audio.owner == DECODED_AUDIO_DR_MP3) {
            drmp3_free(audio.allocation, NULL);
        }
    }
    ResetDecodedAudio(audio);
}

static bool FinishDecodedAudio(DecodedAudio& out, void* samples,
                               drmp3_uint64 frames, u32 channels,
                               u32 sampleRate, int owner) {
    const drmp3_uint64 bytes = frames * channels * sizeof(drmp3_int16);
    if (!samples || frames == 0 || channels == 0 || channels > 2 ||
        sampleRate == 0 || bytes > 0xFFFFFFFFu) {
        if (samples) {
            if (owner == DECODED_AUDIO_DR_WAV) drwav_free(samples, NULL);
            else drmp3_free(samples, NULL);
        }
        return false;
    }
    out.data = reinterpret_cast<u8*>(samples);
    out.dataSize = (u32)bytes;
    out.sampleRate = sampleRate;
    out.channels = channels;
    out.allocation = samples;
    out.owner = owner;
    return true;
}

static bool DecodeWavMemory(const u8* data, u32 size, DecodedAudio& out) {
    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 frames = 0;
    drwav_int16* samples = drwav_open_memory_and_read_pcm_frames_s16(
        data, (size_t)size, &channels, &sampleRate, &frames, NULL);
    return FinishDecodedAudio(out, samples, frames, channels, sampleRate,
                              DECODED_AUDIO_DR_WAV);
}

static bool DecodeMp3Memory(const u8* data, u32 size, DecodedAudio& out) {
    drmp3_config config;
    memset(&config, 0, sizeof(config));
    drmp3_uint64 frames = 0;
    drmp3_int16* samples = drmp3_open_memory_and_read_pcm_frames_s16(
        data, (size_t)size, &config, &frames, NULL);
    return FinishDecodedAudio(out, samples, frames, config.channels,
                              config.sampleRate, DECODED_AUDIO_DR_MP3);
}

bool DecodeAudioMemory(const u8* data, u32 size, DecodedAudio& out) {
    ResetDecodedAudio(out);
    if (!data || size < 4) return false;

    if (size >= 12 &&
        ((memcmp(data, "RIFF", 4) == 0) ||
         (memcmp(data, "RIFX", 4) == 0) ||
         (memcmp(data, "RF64", 4) == 0))) {
        return DecodeWavMemory(data, size, out);
    }
    if (memcmp(data, "OggS", 4) == 0) return false;
    return DecodeMp3Memory(data, size, out);
}

} // namespace fnaf
