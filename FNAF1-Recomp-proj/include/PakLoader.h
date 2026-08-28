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
    u8* data;
    u32 dataSize;
    u32 format; // 0 PCM, 1 XMA2
    u32 sampleRate;
    u32 channels;
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

private:
    void* m_device;
    PakLoadedTexture* m_textures;
    int m_numTextures;
    PakLoadedSound* m_sounds;
    int m_numSounds;
    u8* m_pakData; // keep file in memory for audio
    u32 m_pakSize;

    u32 LoadBE32(const u8* p);
};

} // namespace fnaf

#endif // FNAF_PAK_LOADER_H
