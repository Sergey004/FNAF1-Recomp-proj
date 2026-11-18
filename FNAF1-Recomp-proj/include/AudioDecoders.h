/**
 * AudioDecoders.h: in-game compressed-audio decoding for XAudio2.
 *
 * The pak keeps ordinary PCM on the existing PakLoader path. Passthrough
 * RIFF/WAVE and MPEG Layer III blobs are decoded here, on first playback.
 * The returned allocation belongs to the caller and remains valid until
 * FreeDecodedAudio() is called (after the XAudio2 voice is destroyed).
 */

#ifndef FNAF_AUDIO_DECODERS_H
#define FNAF_AUDIO_DECODERS_H

#include "Types.h"

namespace fnaf {

enum DecodedAudioOwner {
    DECODED_AUDIO_NONE = 0,
    DECODED_AUDIO_DR_WAV = 1,
    DECODED_AUDIO_DR_MP3 = 2
};

struct DecodedAudio {
    u8* data;                 // interleaved signed PCM16, native byte order
    u32 dataSize;
    u32 sampleRate;
    u32 channels;
    void* allocation;         // dr_libs-owned allocation
    int owner;                // DecodedAudioOwner
};

void ResetDecodedAudio(DecodedAudio& audio);
void FreeDecodedAudio(DecodedAudio& audio);

// RIFF/RIFX/RF64 is routed to dr_wav; other input is offered to dr_mp3.
// Ogg/Vorbis is rejected explicitly rather than being mistaken for MP3.
bool DecodeAudioMemory(const u8* data, u32 size, DecodedAudio& out);

} // namespace fnaf

#endif // FNAF_AUDIO_DECODERS_H
