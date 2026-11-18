// Host-side probe for the exact decoder adapter compiled into the XDK game.
// Usage: audio_decode_probe <input> <decoded-s16-output>
#include "AudioDecoders.h"

#include <cstdio>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    FILE* in = std::fopen(argv[1], "rb");
    if (!in) return 3;
    std::fseek(in, 0, SEEK_END);
    const long length = std::ftell(in);
    std::fseek(in, 0, SEEK_SET);
    if (length <= 0 || (unsigned long)length > 0xFFFFFFFFul) {
        std::fclose(in);
        return 4;
    }
    std::vector<fnaf::u8> bytes((size_t)length);
    const size_t got = std::fread(bytes.data(), 1, bytes.size(), in);
    std::fclose(in);
    if (got != bytes.size()) return 5;

    fnaf::DecodedAudio decoded;
    if (!fnaf::DecodeAudioMemory(bytes.data(), (fnaf::u32)bytes.size(), decoded)) {
        std::fprintf(stderr, "decode failed: %s\n", argv[1]);
        return 6;
    }
    FILE* out = std::fopen(argv[2], "wb");
    if (!out) {
        fnaf::FreeDecodedAudio(decoded);
        return 7;
    }
    const size_t wrote = std::fwrite(decoded.data, 1, decoded.dataSize, out);
    std::fclose(out);
    std::printf("%u Hz, %u ch, %u bytes\n",
                decoded.sampleRate, decoded.channels, decoded.dataSize);
    const bool complete = (wrote == decoded.dataSize);
    fnaf::FreeDecodedAudio(decoded);
    return complete ? 0 : 8;
}
