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
    // v2.29 streaming mode: where the blob lives in the pak file and whether
    // the D3D texture has been created yet (lazy GPU upload).
    u32 fileOff;
    u32 dataSize;
    bool gpuReady;
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
    // v2.29 streaming mode: blob location in the pak file; resident=false
    // until the first FindSound/Play reads it into an owned buffer.
    u32  fileOff;
    bool resident;
    u32  pakFmt;     // raw pak table format (0 PCM, 2 passthrough e.g. mp3)
};

class PakLoader {
public:
    PakLoader();
    ~PakLoader();

    bool Load(const char* pakPath, void* device);

    // v2.29: STREAMING load — built for Sister Location (its pak is
    // ~1.5 GB, impossible to slurp into the 512 MB UMA pool). Instead of
    // reading the whole file we keep it open and read each asset's blob
    // at its table offset on first use ("sliding" over the file).
    // Header + tables + name pool stay resident (a few dozen KB); the
    // tex/snd DATA is never read up front:
    //   * textures — created/uploaded on the first FindTexture, source
    //     blob freed right after (UMA: once it is in the D3D texture it
    //     lives in the same 512 MB pool the GPU reads);
    //   * sounds — read + normalized (RIFF peel / byte order / 8-bit
    //     expand; passthrough blobs like SL's mp3s stay unplayable) on
    //     the first FindSound into an owned buffer;
    //   * PreloadAsync() pushes a set of texture names to a background
    //     thread (XDK) so a room-to-room transition pulls the next
    //     room's assets while the frame renders. PC builds fall back to
    //     synchronous loads.
    // FNAF1/2/3/4 stay on the eager Load() — their paks fit comfortably.
    // The SL module (stage 7 in docs/ARCHITECTURE.md) activates this.
    bool LoadStreaming(const char* pakPath, void* device);

    // v2.29: queue names for background ensure-loaded (streaming mode only;
    // no-op in eager mode). Names not found are silently skipped.
    void PreloadAsync(const char* const* texNames, int count);
    bool IsStreaming() const { return m_streaming; }

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

    // ---- v2.29 streaming mode internals ----
    void* m_file;            // FILE* of the open pak (streaming mode)
    bool  m_streaming;
    bool  m_swapAll;         // pak-wide PCM byte-order verdict (mini-vote at load)
    // background preload (XDK): ring queue of texture indices + worker
    void  EnsureTextureLoaded(int idx);
    void  EnsureSoundLoaded(int idx);
    bool  ReadAt(u32 off, u32 size, u8* dst);   // CS-guarded seek+read
    void  UploadTexture(int idx, const u8* blob);
    void  NormalizeOne(int idx);                // per-sound, after its blob lands
    void  PickVoteCandidates();                 // choose sounds for the mini-vote
    int   m_voteIdx[8];                         // candidate sounds for the vote
    int   m_voteCount;
    void* m_thread;                             // HANDLE (XDK)
    void* m_wakeEvent;                          // HANDLE (XDK)
    void* m_cs;                                 // CRITICAL_SECTION* (XDK)
    volatile long m_queue[64];                  // texture indices
    volatile int  m_qHead, m_qTail;
    volatile bool m_quit;
    static unsigned long PreloadThreadStatic(void* param); // XDK worker
    void PreloadThreadRun();
};

} // namespace fnaf

#endif // FNAF_PAK_LOADER_H
