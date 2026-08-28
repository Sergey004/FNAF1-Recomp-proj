/**
 * PakLoader.cpp: Xbox 360 .pak loader
 */

#include "PakLoader.h"
#include <cstdio>
#include <cstring>

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#endif

namespace fnaf {

PakLoader::PakLoader()
    : m_device(0)
    , m_textures(0)
    , m_numTextures(0)
    , m_sounds(0)
    , m_numSounds(0)
    , m_pakData(0)
    , m_pakSize(0)
{
}

PakLoader::~PakLoader()
{
    Unload();
}

u32 PakLoader::LoadBE32(const u8* p)
{
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

void PakLoader::Unload()
{
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    if (m_textures) {
        for (int i = 0; i < m_numTextures; ++i) {
            if (m_textures[i].texture) {
                ((IDirect3DTexture9*)m_textures[i].texture)->Release();
                m_textures[i].texture = 0;
            }
        }
        delete[] m_textures;
        m_textures = 0;
    }
    if (m_sounds) {
        // data is inside m_pakData, not owned separately
        delete[] m_sounds;
        m_sounds = 0;
    }
    if (m_pakData) {
        // Use XPhysicalAlloc? For now free with delete
        delete[] m_pakData;
        m_pakData = 0;
    }
#else
    if (m_textures) delete[] m_textures;
    if (m_sounds) delete[] m_sounds;
    if (m_pakData) delete[] m_pakData;
    m_textures = 0; m_sounds = 0; m_pakData = 0;
#endif
    m_numTextures = 0;
    m_numSounds = 0;
    m_device = 0;
    m_pakSize = 0;
}

bool PakLoader::Load(const char* pakPath, void* device)
{
    if (!pakPath || !device) return false;
    Unload();
    m_device = device;

    // Load file into memory (Xbox: use fopen, for large pak use XPhysicalAlloc)
    FILE* f = fopen(pakPath, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    m_pakSize = (u32)sz;
    m_pakData = new u8[m_pakSize];
    if (!m_pakData) { fclose(f); return false; }
    if ((long)fread(m_pakData, 1, m_pakSize, f) != sz) { fclose(f); Unload(); return false; }
    fclose(f);

    const u8* p = m_pakData;
    u32 magic = LoadBE32(p); p += 4;
    if (magic != 0x464E4146) { Unload(); return false; } // 'FNAF'
    u32 version = LoadBE32(p); p += 4;
    u32 numTex = LoadBE32(p); p += 4;
    u32 numSnd = LoadBE32(p); p += 4;
    u32 numMus = LoadBE32(p); p += 4;
    u32 numFnt = LoadBE32(p); p += 4;
    u32 stringsOffset = LoadBE32(p); p += 4;
    u32 dataOffset = LoadBE32(p); p += 4;
    (void)version; (void)numMus; (void)numFnt;

    m_numTextures = (int)numTex;
    m_numSounds = (int)numSnd;
    if (m_numTextures > 0) m_textures = new PakLoadedTexture[m_numTextures];
    if (m_numSounds > 0) m_sounds = new PakLoadedSound[m_numSounds];
    if (m_numTextures>0) memset(m_textures, 0, sizeof(PakLoadedTexture)*m_numTextures);
    if (m_numSounds>0) memset(m_sounds, 0, sizeof(PakLoadedSound)*m_numSounds);

    // Texture entries: 9 *4 =36 bytes each
    for (int i = 0; i < m_numTextures; ++i) {
        u32 nameOff = LoadBE32(p); p+=4;
        u32 dataOff = LoadBE32(p); p+=4;
        u32 dataSize = LoadBE32(p); p+=4;
        u32 fmt = LoadBE32(p); p+=4;
        u32 origW = LoadBE32(p); p+=4;
        u32 origH = LoadBE32(p); p+=4;
        u32 alignW = LoadBE32(p); p+=4;
        u32 alignH = LoadBE32(p); p+=4;
        u32 mip = LoadBE32(p); p+=4;

        const char* nameStr = (const char*)(m_pakData + nameOff);
        strncpy(m_textures[i].name, nameStr, 63);
        m_textures[i].name[63] = '\0';
        m_textures[i].origWidth = origW;
        m_textures[i].origHeight = origH;
        m_textures[i].alignedWidth = alignW;
        m_textures[i].alignedHeight = alignH;
        m_textures[i].format = fmt;

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
        D3DFORMAT d3dFmt = D3DFMT_A8R8G8B8;
        if (fmt == 0) d3dFmt = D3DFMT_DXT1;
        else if (fmt == 2) d3dFmt = D3DFMT_DXT5;
        else if (fmt == 3) d3dFmt = D3DFMT_A8R8G8B8;

        // Create texture - note: for tiled textures, we need to use XGSetTextureHeader? For simplicity use CreateTexture and copy tiled data directly
        // Xbox XDK: tiled data can be memcpy into locked rect if texture was created with correct tiling (handled by driver)
        // Our data is already tiled+BE swapped, so direct memcpy works
        IDirect3DTexture9* tex = 0;
        HRESULT hr = ((D3DDevice*)m_device)->CreateTexture(alignW, alignH, 1, 0, d3dFmt, D3DPOOL_DEFAULT, &tex, NULL);
        m_textures[i].texture = (void*)tex;
        if (SUCCEEDED(hr) && m_textures[i].texture) {
            D3DLOCKED_RECT lr;
            if (SUCCEEDED(((IDirect3DTexture9*)m_textures[i].texture)->LockRect(0, &lr, NULL, 0))) {
                const u8* src = m_pakData + dataOff;
                memcpy(lr.pBits, src, dataSize);
                ((IDirect3DTexture9*)m_textures[i].texture)->UnlockRect(0);
            }
        }
#else
        (void)dataOff; (void)dataSize;
        m_textures[i].texture = 0;
#endif
        (void)mip;
    }

    // Sound entries: 8*4=32 bytes each
    for (int i = 0; i < m_numSounds; ++i) {
        u32 nameOff = LoadBE32(p); p+=4;
        u32 dataOff = LoadBE32(p); p+=4;
        u32 dataSize = LoadBE32(p); p+=4;
        u32 fmt = LoadBE32(p); p+=4;
        u32 sr = LoadBE32(p); p+=4;
        u32 ch = LoadBE32(p); p+=4;
        u32 loopS = LoadBE32(p); p+=4;
        u32 loopE = LoadBE32(p); p+=4;
        (void)loopS; (void)loopE;
        const char* nameStr = (const char*)(m_pakData + nameOff);
        strncpy(m_sounds[i].name, nameStr, 63);
        m_sounds[i].name[63] = '\0';
        m_sounds[i].data = (u8*)(m_pakData + dataOff);
        m_sounds[i].dataSize = dataSize;
        m_sounds[i].format = fmt;
        m_sounds[i].sampleRate = sr;
        m_sounds[i].channels = ch;
    }

    (void)stringsOffset; (void)dataOffset;
    return true;
}

int PakLoader::GetTextureCount() const { return m_numTextures; }
PakLoadedTexture* PakLoader::GetTexture(int idx) { if(idx<0||idx>=m_numTextures) return 0; return &m_textures[idx]; }
PakLoadedTexture* PakLoader::FindTexture(const char* name) {
    if(!name) return 0;
    for(int i=0;i<m_numTextures;++i) if(strcmp(m_textures[i].name, name)==0) return &m_textures[i];
    return 0;
}
int PakLoader::GetSoundCount() const { return m_numSounds; }
PakLoadedSound* PakLoader::GetSound(int idx) { if(idx<0||idx>=m_numSounds) return 0; return &m_sounds[idx]; }
PakLoadedSound* PakLoader::FindSound(const char* name) {
    if(!name) return 0;
    for(int i=0;i<m_numSounds;++i) if(strcmp(m_sounds[i].name, name)==0) return &m_sounds[i];
    return 0;
}

} // namespace fnaf
