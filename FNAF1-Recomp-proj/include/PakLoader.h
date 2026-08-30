/**
 * PakLoader.h: Xbox 360 .pak loader (Big-Endian)
 * VS2010 compatible
 */

#ifndef FNAF_PAK_LOADER_H
#define FNAF_PAK_LOADER_H

#include "Types.h"

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
typedef D3DDevice D3DDeviceX;
typedef D3DTexture D3DTextureX;
#else
struct IDirect3DDevice9; typedef IDirect3DDevice9 D3DDeviceX;
struct IDirect3DTexture9; typedef IDirect3DTexture9 D3DTextureX;
#endif

namespace fnaf {

struct PakLoadedTexture {
    char name[64];
    void* texture;
    u32 origWidth;
    u32 origHeight;
    u32 alignedWidth;
    u32 alignedHeight;
    u32 format;
};

struct PakLoadedSound {
    char name[64];
    u8*  data;       // v2.6: start of PLAYABLE PCM (data-chunk payload when the
                     //       blob is a complete RIFF/WAVE file, blob otherwise)
    u32  dataSize;
    u32  format;     // 0 = PCM16 ready to play, 1 = unsupported codec
    u32  sampleRate;
    u32  channels;
    u32  bits;       // v2.6: source bit depth (8/16); 8-bit payloads are
                     //       expanded to 16-bit at load, so Play() always
                     //       receives 16-bit little-endian data
};

class PakLoader {
public:
    PakLoader();
    ~PakLoader();

    bool Load(const char* pakPath, void* device);
    void Unload();

    int GetTextureCount() const;
    PakLoadedTexture* GetTexture(int idx);
    PakLoadedTexture* FindTexture(const char* name);

    int GetSoundCount() const;
    PakLoadedSound* GetSound(int idx);
    PakLoadedSound* FindSound(const char* name);

    // v2.6: sound-normalization statistics (for the debug overlay / log):
    // how many blobs carried a RIFF header, how many 16-bit payloads were
    // byte-swapped (pak written with big-endian PCM), how many 8-bit
    // payloads were expanded.
    int GetSndRiffCount() const   { return m_sndRiff; }
    int GetSndSwappedCount() const { return m_sndSwapped; }
    int GetSnd8bitCount() const   { return m_snd8bit; }

private:
    void* m_device;
    PakLoadedTexture* m_textures;
    int m_numTextures;
    PakLoadedSound* m_sounds;
    int m_numSounds;
    u8* m_pakData; // keep file in memory for audio
    u32 m_pakSize;

    // v2.6: owned 16-bit expansion buffers for 8-bit sounds (freed in Unload)
    struct OwnedBuf { u8* data; struct OwnedBuf* next; };
    OwnedBuf* m_owned;

    // v2.6: normalization stats
    int m_sndRiff;
    int m_sndSwapped;
    int m_snd8bit;

    u32 LoadBE32(const u8* p);
    void NormalizeSounds();
    void FreeOwned();
};

} // namespace fnaf

#endif // FNAF_PAK_LOADER_H
