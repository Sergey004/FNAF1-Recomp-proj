/**
 * PakLoader.cpp: Xbox 360 .pak loader
 */

#include "PakLoader.h"
#include <cstdio>
#include <cstring>

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
#include <xtl.h>
#include <d3d9.h>
#include <xgraphics.h>   // XGGetGpuFormat, XGTileTextureLevel
#endif

namespace fnaf {

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
// ------------------------------------------------------------------
//  v2.3: DXT byte-order conversion (pak layout -> Xenos GPU layout)
//
//  Official XDK references:
//   * "Big-Endian vs. Little-Endian" (XDK docs): resources authored on
//     PC are little-endian and need a swap to be consumed by the 360;
//     DXT textures use GPUENDIAN_8IN16, i.e. an 8-in-16 swap -- ADJACENT
//     BYTE PAIRS, not whole words ("abcdefgh" -> "badcfehg").
//   * "Xbox 360 Texture and Render Target Layouts" (ATG white paper):
//     a DXT5A block stored on the 360 is "80 82 ..." with Anchor0=0x82
//     and Anchor1=0x80 -- i.e. the SECOND anchor byte comes first in
//     memory, exactly what an 8-in-16 swap of the PC layout produces.
//   * XDK d3d9types.h: D3DFMT_DXT1/DXT4/5 are defined with
//     GPUENDIAN_8IN16; D3DFMT_A8R8G8B8 with GPUENDIAN_8IN32.
//
//  The pak was built with "whole-word big-endian" DXT fields, which is
//  a DIFFERENT permutation from the 8-in-16 swap the GPU expects:
//
//                      PC/LE bytes       pak (BE words)    GPU (8-in-16)
//    DXT5 anchors      a0 a1             a0 a1             a1 a0
//    DXT5 alpha idx    i0..i5            i5..i0 (reverse)  i1 i0 i3 i2 i5 i4
//    DXT* color u16s   lo hi             hi lo             hi lo        (same)
//    DXT* color idx    i0 i1 i2 i3       i3 i2 i1 i0       i1 i0 i3 i2
//
//  ConvertBlock converts each pak block into the GPU layout in place.
// ------------------------------------------------------------------

static void ConvertDxt1Block(u8* b)
{
    // bytes 0..3: color endpoints (u16 BE == 8-in-16 form) -- unchanged.
    // bytes 4..7: pak [i3 i2 i1 i0] -> GPU [i1 i0 i3 i2].
    u8 t;
    t = b[4]; b[4] = b[6]; b[6] = t;   // swap 16-bit halves
    t = b[5]; b[5] = b[7]; b[7] = t;
}

static void ConvertDxt5Block(u8* b)
{
    // bytes 0..1: alpha anchors, pak [a0 a1] -> GPU [a1 a0].
    u8 t = b[0]; b[0] = b[1]; b[1] = t;
    // bytes 2..7: alpha indices, pak [i5 i4 i3 i2 i1 i0] ->
    //             GPU [i1 i0 i3 i2 i5 i4].
    u8 tmp[6];
    for (int k = 0; k < 6; ++k) tmp[k] = b[2 + k];       // i5 i4 i3 i2 i1 i0
    b[2] = tmp[4]; b[3] = tmp[5];                        // i1 i0
    b[4] = tmp[2]; b[5] = tmp[3];                        // i3 i2
    b[6] = tmp[0]; b[7] = tmp[1];                        // i5 i4
    // bytes 8..11: color endpoints -- unchanged.
    // bytes 12..15: color indices, same permutation as DXT1.
    t = b[12]; b[12] = b[14]; b[14] = t;
    t = b[13]; b[13] = b[15]; b[15] = t;
}

// Convert a whole pak DXT image (rows of blocks, tightly packed) from the
// pak's big-endian-word encoding to the GPU's 8-in-16 layout.
static void ConvertDxtPakToGpu(u8* data, u32 sizeBytes, bool dxt5)
{
    const u32 block = dxt5 ? 16u : 8u;
    for (u32 off = 0; off + block <= sizeBytes; off += block) {
        if (dxt5) ConvertDxt5Block(data + off);
        else      ConvertDxt1Block(data + off);
    }
}
#endif

PakLoader::PakLoader()
    : m_device(0)
    , m_textures(0)
    , m_numTextures(0)
    , m_sounds(0)
    , m_numSounds(0)
    , m_pakData(0)
    , m_pakSize(0)
    , m_owned(0)
    , m_sndRiff(0)
    , m_sndSwapped(0)
    , m_snd8bit(0)
    , m_file(0)            // v2.29 streaming state
    , m_streaming(false)
    , m_swapAll(false)
    , m_voteCount(0)
    , m_thread(0)
    , m_wakeEvent(0)
    , m_cs(0)
    , m_qHead(0)
    , m_qTail(0)
    , m_quit(false)
{
}

// =====================================================================
//  v2.6 sound normalization
//  -------------------------------------------------------------------
//  The pak container is big-endian by convention: the header fields are
//  BE32 (see LoadBE32) and the texture blobs are stored with byte-swapped
//  DXT words (converted back at upload, see ConvertDxtPakToGpu above).
//  Depending on which tool wrote a given fnaf1.pak, the sound payloads
//  inside it may be:
//
//    1) headerless LITTLE-endian PCM16  (what AudioSystem expects)
//    2) headerless BIG-endian PCM16     (played as LE = pure noise!)
//    3) a complete RIFF/WAVE file       (44..172 byte headers: some of the
//                                       original WAVs carry LIST/INFO
//                                       chunks before 'data')
//    4) a RIFF file with byte-swapped payloads
//    5) 8-bit PCM (XSCREAM) presented as 16-bit (noise again)
//
//  NormalizeSounds() runs once at Load() and turns every entry into
//  case 1, so AudioSystem can stay dumb:
//    * SndRiffParse() walks the chunk list when the blob begins with
//      'RIFF'....'WAVE' (chunk sizes may be LE or BE) and re-points the
//      entry at the 'data' payload with the real rate/channels/bits;
//    * 16-bit payloads are byte-order tested by comparing the mean
//      |amplitude| of the stream read BOTH ways (the wrong order moves
//      the near-uniform low bytes into the high position and inflates
//      the amplitude several-fold; measured margin on the real bank is
//      3.1x..88x for 50/51 sounds). The pak votes as a WHOLE so a single
//      loud edge-case sound cannot flip the decision, and the literal
//      white-noise loops (static/static2) do not vote;
//    * 8-bit payloads are expanded to 16-bit into owned buffers
//      (XAudio2 2.x on the 360 has no 8-bit sample format).
// =====================================================================

static u32 SndRdLE32(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}
static u16 SndRdLE16(const u8* p) {
    return (u16)((u32)p[0] | ((u32)p[1] << 8));
}
static u32 SndRdBE32(const u8* p) {
    return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | (u32)p[3];
}

struct SndRiffInfo {
    u32 tag;        // 1 = PCM
    u32 channels;
    u32 rate;
    u32 bits;
    u32 dataOff;    // payload offset inside the blob
    u32 dataSize;   // payload size
};

// Walk a RIFF/WAVE chunk list. Tolerates byte-swapped (BE) size fields:
// the first size that fits inside the blob wins.
static bool SndRiffParse(const u8* p, u32 size, SndRiffInfo& out) {
    if (size < 64) return false;
    if (p[0] != 'R' || p[1] != 'I' || p[2] != 'F' || p[3] != 'F') return false;
    if (p[8] != 'W' || p[9] != 'A' || p[10] != 'V' || p[11] != 'E') return false;

    SndRiffInfo r;
    r.tag = 1; r.channels = 0; r.rate = 0; r.bits = 0;
    r.dataOff = 0; r.dataSize = 0;
    bool gotFmt = false, gotData = false;

    u32 pos = 12;
    while (pos + 8 <= size) {
        const u8* id = p + pos;
        const u32 maxSz = size - pos - 8;
        u32 sz = SndRdLE32(p + pos + 4);
        if (sz > maxSz) {
            // try big-endian size before giving up
            const u32 szBE = SndRdBE32(p + pos + 4);
            if (szBE <= maxSz) sz = szBE;
            else return false;
        }
        if (id[0] == 'f' && id[1] == 'm' && id[2] == 't' && id[3] == ' ' && sz >= 16) {
            const u8* f = p + pos + 8;
            r.tag      = SndRdLE16(f + 0);
            r.channels = SndRdLE16(f + 2);
            r.rate     = SndRdLE32(f + 4);
            r.bits     = SndRdLE16(f + 14);
            gotFmt = true;
            // WAVE_FORMAT_EXTENSIBLE: the real tag hides in SubFormat GUID
            if (r.tag == 0xFFFE && sz >= 40) r.tag = SndRdLE16(f + 24);
        } else if (id[0] == 'd' && id[1] == 'a' && id[2] == 't' && id[3] == 'a') {
            r.dataOff = pos + 8;
            r.dataSize = sz <= (size - r.dataOff) ? sz : (size - r.dataOff);
            gotData = true;
            break;
        }
        pos += 8 + sz + (sz & 1);
    }
    if (!gotFmt || !gotData || r.dataSize == 0) return false;
    if (r.tag != 1) return false;                 // PCM only
    if (r.channels == 0 || r.channels > 2) return false;
    if (r.rate < 3000 || r.rate > 96000) return false;
    if (r.bits != 8 && r.bits != 16) return false;
    out = r;
    return true;
}

// Byte-order probe.
//
// Real audio stores the waveform COARSE value in the high byte and fine
// detail in the low byte. Reading the stream in the wrong order moves the
// (near-uniform, large) low bytes into the high position, which always
// inflates the mean amplitude several-fold. We therefore read the mean
// |sample| BOTH ways; the smaller reading is the true one.
//
// Measured on the real FNaF1 bank (52 sounds, see tools/fix_pak_sounds.py
// report): 50/51 sounds separate by 3.1x..88x. The only near-tie is
// XSCREAM2 (0.97x, a near-full-scale scream) — it abstains here and is
// covered by the pak-wide majority vote instead of its own probe.
//   returns  1 = looks BIG-endian,  0 = looks little-endian,
//           -1 = abstain (silence or ambiguous)
static int SndPcmByteOrderVote(const u8* p, u32 bytes) {
    const u32 n = bytes / 2;
    if (n < 512) return -1;                       // too short to judge
    u32 scan = n < 2000000u ? n : 2000000u;       // cap at ~4 MB of work
    u64 sumLE = 0, sumBE = 0;
    for (u32 i = 0; i < scan; ++i) {
        const u32 lo = p[i * 2 + 0];
        const u32 hi = p[i * 2 + 1];
        // |s16| of the LE reading (v = lo | hi<<8)
        const u32 vLE = lo | (hi << 8);
        sumLE += (vLE & 0x8000) ? (65536u - vLE) : vLE;
        // |s16| of the BE reading (v = hi | lo<<8)
        const u32 vBE = hi | (lo << 8);
        sumBE += (vBE & 0x8000) ? (65536u - vBE) : vBE;
    }
    const f64 le = (f64)sumLE / scan;
    const f64 be = (f64)sumBE / scan;
    if (le < 4.0 && be < 4.0) return -1;          // digital silence
    if (be < le * 0.8) return 1;                  // BE reading much quieter -> stored BE
    if (le < be * 0.8) return 0;                  // LE reading much quieter -> stored LE
    return -1;                                    // near-tie: abstain
}

static void SndSwap16InPlace(u8* p, u32 bytes) {
    const u32 n = bytes / 2;
    for (u32 i = 0; i < n; ++i) {
        u8 t = p[i * 2]; p[i * 2] = p[i * 2 + 1]; p[i * 2 + 1] = t;
    }
}

// Literal white-noise loops: they look like noise in both byte orders and
// must not vote (their name always contains "static").
static bool SndIsLiteralNoise(const char* name) {
    for (const char* s = name; *s; ++s) {
        if (s[0] == 's' && s[1] == 't' && s[2] == 'a' && s[3] == 't' &&
            s[4] == 'i' && s[5] == 'c') return true;
    }
    return false;
}

// The original XSCREAM is the ONLY 8-bit sound in the whole bank
// (115316 u8 samples, see the sound dump). When it arrives headerless the
// pak entry cannot express the bit depth, so match it by name+size.
static bool SndIsKnown8bit(const char* name, u32 bytes) {
    const char* x = "XSCREAM";
    // match "snd_XSCREAM" / "XSCREAM" anywhere in the name
    for (const char* s = name; *s; ++s) {
        int i = 0;
        while (x[i] && s[i] == x[i]) ++i;
        if (!x[i]) return bytes == 115316 || bytes == 115360;
        if (!s[i]) break;
    }
    return false;
}

void PakLoader::FreeOwned() {
    OwnedBuf* b = m_owned;
    while (b) {
        OwnedBuf* nx = b->next;
        delete[] b->data;
        delete b;
        b = nx;
    }
    m_owned = 0;
}

void PakLoader::NormalizeSounds() {
    if (!m_sounds || m_numSounds <= 0) return;

    // ---- pass 1: peel RIFF containers --------------------------------
    for (int i = 0; i < m_numSounds; ++i) {
        PakLoadedSound& s = m_sounds[i];
        s.bits = 16;
        if (!s.data || s.dataSize < 64) continue;
        SndRiffInfo ri;
        if (SndRiffParse(s.data, s.dataSize, ri)) {
            s.data      += ri.dataOff;
            s.dataSize   = ri.dataSize;
            s.sampleRate = ri.rate;
            s.channels   = ri.channels;
            s.bits       = ri.bits;
            ++m_sndRiff;
        }
    }

    // ---- pass 2: pak-wide byte-order vote -----------------------------
    // A pak written by a big-endian tool has ALL 16-bit payloads swapped;
    // deciding once avoids mis-flagging one unusual sound in isolation.
    // (The probe abstains on silence and near-ties like XSCREAM2; those
    // are covered by the majority of the remaining bank.)
    int voteSwap = 0, voteNative = 0;
    for (int i = 0; i < m_numSounds; ++i) {
        const PakLoadedSound& s = m_sounds[i];
        if (s.format != 0 || !s.data) continue;
        if (s.bits != 16) continue;
        if (SndIsLiteralNoise(s.name)) continue;
        if (SndIsKnown8bit(s.name, s.dataSize)) continue;
        const int v = SndPcmByteOrderVote(s.data, s.dataSize);
        if (v > 0) ++voteSwap;
        else if (v == 0) ++voteNative;
    }
    const bool swapAll = (voteSwap > voteNative) && voteSwap > 0;

    // ---- pass 3: fix what needs fixing ---------------------------------
    for (int i = 0; i < m_numSounds; ++i) {
        PakLoadedSound& s = m_sounds[i];
        if (!s.data || s.dataSize == 0) { s.format = 1; continue; }

        if (s.bits == 8 || SndIsKnown8bit(s.name, s.dataSize)) {
            // expand u8 -> s16 into an owned buffer
            const u32 n = s.dataSize;             // one byte per sample
            u8* out = new u8[n * 2];
            if (!out) { s.format = 1; continue; }
            for (u32 k = 0; k < n; ++k) {
                const int v = ((int)s.data[k] - 128) << 8;
                out[k * 2 + 0] = (u8)(v & 0xFF);
                out[k * 2 + 1] = (u8)((v >> 8) & 0xFF);
            }
            OwnedBuf* b = new OwnedBuf;
            b->data = out; b->next = m_owned; m_owned = b;
            s.data = out;
            s.dataSize = n * 2;
            s.bits = 16;
            s.format = 0;
            ++m_snd8bit;
            continue;
        }

        if (s.bits != 16) { s.format = 1; continue; }  // unknown depth
        if (swapAll) { SndSwap16InPlace(s.data, s.dataSize); ++m_sndSwapped; }
        s.format = 0;
    }

    // ---- pass 4: final byte order for THIS platform ---------------------
    // XDK doc "Audio Data and Endianness" (audio_overview_xaudio2_
    // endianness.htm): "On Xbox 360, PCM audio whose WAVEFORMATEX format
    // contains a wBitsPerSample value of 16 or 32 should byte-swap its
    // audio data. If wBitsPerSample is 16, each pair of bytes in the audio
    // data should be exchanged." The 360 XAudio2 mixes on the big-endian
    // PPC and does no conversion of its own — feeding it the PC-convention
    // little-endian PCM16 produced by passes 1-3 plays as pure noise
    // (exactly the "noise is still there" report from the console build).
    // So the normalization TARGET is platform-dependent: LE on PC, BE on
    // the 360. The 8-bit expansions above were written LE; they get
    // swapped here like everything else. Not counted in m_sndSwapped
    // (that stat tracks what the PAK had stored).
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    for (int i = 0; i < m_numSounds; ++i) {
        PakLoadedSound& s = m_sounds[i];
        if (s.format != 0 || s.bits != 16) continue;
        if (!s.data || s.dataSize < 2) continue;
        SndSwap16InPlace(s.data, s.dataSize);
    }
#endif

    // v2.7.4: name the normalization target in the log so the running XEX
    // is diagnosable from the console output alone
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    const char* sndTarget = "BE(360)";
#else
    const char* sndTarget = "LE(PC)";
#endif
    printf("PakLoader: sounds normalized (riff=%d swapped=%d 8bit=%d, vote %d/%d, target %s)\n",
           m_sndRiff, m_sndSwapped, m_snd8bit, voteSwap, voteNative, sndTarget);
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
    // v2.29: stop the streaming preload worker first
    if (m_thread || m_wakeEvent) {
        m_quit = true;
        if (m_wakeEvent) SetEvent((HANDLE)m_wakeEvent);
        if (m_thread) { WaitForSingleObject((HANDLE)m_thread, 2000); CloseHandle((HANDLE)m_thread); }
        if (m_wakeEvent) CloseHandle((HANDLE)m_wakeEvent);
        m_thread = 0; m_wakeEvent = 0;
    }
    if (m_cs) { DeleteCriticalSection((CRITICAL_SECTION*)m_cs); delete (CRITICAL_SECTION*)m_cs; m_cs = 0; }
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
    FreeOwned();
#else
    if (m_textures) delete[] m_textures;
    if (m_sounds) delete[] m_sounds;
    if (m_pakData) delete[] m_pakData;
    m_textures = 0; m_sounds = 0; m_pakData = 0;
    FreeOwned();
#endif
    m_numTextures = 0;
    m_numSounds = 0;
    m_device = 0;
    m_pakSize = 0;
    m_sndRiff = 0;
    m_sndSwapped = 0;
    m_snd8bit = 0;
    // v2.29 streaming state
    if (m_file) { fclose((FILE*)m_file); m_file = 0; }
    m_streaming = false;
    m_swapAll = false;
    m_voteCount = 0;
    m_qHead = m_qTail = 0;
    m_quit = false;
}

// ---- v2.29: GPU upload for one texture entry, shared by the eager and
// streaming paths. `blob` is the raw pak payload (in-RAM pointer in eager
// mode, staging buffer in streaming mode). ----
void PakLoader::UploadTexture(int idx, const u8* blob)
{
    PakLoadedTexture& t = m_textures[idx];
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    const u32 fmt = t.format;
    const u32 alignW = t.alignedWidth;
    const u32 alignH = t.alignedHeight;
    const u32 dataSize = t.dataSize;

    D3DFORMAT d3dFmt = D3DFMT_A8R8G8B8;
    if (fmt == 0) d3dFmt = D3DFMT_DXT1;
    else if (fmt == 2) d3dFmt = D3DFMT_DXT5;
    else if (fmt == 3) d3dFmt = D3DFMT_A8R8G8B8;

    // v2.3: Xenos textures created with D3DFMT_DXT1/DXT5/A8R8G8B8 are
    // TILED. LockRect hands out the raw tiled bits -- a raw memcpy of
    // linear pak data produced garbled pixels. The XDK utility
    // XGTileTextureLevel swizzles a linear source image into the locked
    // (tiled) level, handling packed miptails and block formats.
    // v2.3 also converts each DXT block from the pak's big-endian-word
    // encoding into the GPU's 8-in-16 layout (see the block comment at
    // the top of this file and docs/XDK_NOTES.md section 8); without it
    // DXT alpha anchors and texel indices decode wrong (garbled art).
    IDirect3DTexture9* tex = 0;
    // v2.3: the pool parameter is documented as "Unused; use 0" on Xbox 360.
    // (D3DPOOL)0 -- MSVC has no implicit int->enum conversion.
    HRESULT hr = ((D3DDevice*)m_device)->CreateTexture(alignW, alignH, 1, 0, d3dFmt, (D3DPOOL)0, &tex, NULL);
    t.texture = (void*)tex;
    if (SUCCEEDED(hr) && tex) {
        D3DLOCKED_RECT lr;
        if (SUCCEEDED(tex->LockRect(0, &lr, NULL, 0))) {
            const u8* src = blob;
            // Source stride: one BLOCK row for the compressed formats
            // (tiled addressing runs in texel/block units, see
            // XGAddress2DTiledOffset), one pixel row for ARGB.
            UINT srcRowPitch;
            switch (d3dFmt) {
                case D3DFMT_DXT1: srcRowPitch = (alignW / 4) * 8;  break; // 4 bpp
                case D3DFMT_DXT5: srcRowPitch = (alignW / 4) * 16; break; // 8 bpp
                default:          srcRowPitch = alignW * 4;        break; // 32 bpp
            }

            // v2.3: convert pak DXT blocks from big-endian-word layout
            // to the GPU's 8-in-16 layout first (see the block comment
            // above ConvertDxt1Block). XGTileTextureLevel copies bytes
            // verbatim -- it does not do any endian fix-up.
            const bool isDxt5 = (d3dFmt == D3DFMT_DXT5);
            const bool isDxt  = (d3dFmt == D3DFMT_DXT1 || isDxt5);
            u8* conv = 0;
            if (isDxt && dataSize > 0 && dataSize <= 64u * 1024u * 1024u) {
                conv = new u8[dataSize];
                memcpy(conv, src, dataSize);
                ConvertDxtPakToGpu(conv, dataSize, isDxt5);
                src = conv;
            }

            XGTileTextureLevel(alignW, alignH, 0, XGGetGpuFormat(d3dFmt), 0,
                               lr.pBits, NULL, src, srcRowPitch, NULL);
            delete[] conv;
            tex->UnlockRect(0);
        }
    }
#endif
    t.gpuReady = true;
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
        m_textures[i].fileOff = dataOff;      // v2.29 (used by streaming mode)
        m_textures[i].dataSize = dataSize;
        m_textures[i].gpuReady = false;

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
        UploadTexture(i, m_pakData + dataOff);
#else
        m_textures[i].gpuReady = true;
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
        m_sounds[i].bits = 16;
        m_sounds[i].fileOff = dataOff;   // v2.29
        m_sounds[i].resident = true;     // eager mode: blob is inside m_pakData
        m_sounds[i].pakFmt = fmt;
    }

    // v2.6: normalize the sound bank into plain little-endian PCM16
    // (peel RIFF headers, fix byte order, expand 8-bit) so AudioSystem
    // can submit every entry to XAudio2 as-is.
    NormalizeSounds();

    (void)stringsOffset; (void)dataOffset;
    return true;
}

// =====================================================================
//  v2.29 STREAMING MODE ("sliding over the file") — built for SL
//  -------------------------------------------------------------------
//  The games run from HDD (RGH/JTAG) and the 360's CPU and GPU share
//  one 512 MB UMA pool. FNAF1/2/3/4 paks fit and stay on the eager
//  Load(); Sister Location's pak is ~1.5 GB — eager is impossible, so
//  its module will boot through LoadStreaming(): keep only the pak
//  header + tables + name pool resident (a few dozen KB) and read each
//  asset's blob at its table offset on first use:
//    * textures: staged read -> CreateTexture/LockRect upload (the same
//      UploadTexture as eager mode) -> staging freed. gpuReady marks the
//      entry; FindTexture ensures synchronously so a render can never
//      see a half-loaded texture;
//    * sounds: read into an owned buffer on the first FindSound and
//      normalized per-sound (RIFF peel / 8-bit expand / byte order).
//      The pak-wide byte-order verdict cannot use the whole bank, so a
//      MINI-VOTE at load time reads the first 4 KB of a few mid-size
//      16-bit sounds and runs the same SndPcmByteOrderVote;
//    * PreloadAsync(): room/camera switches push a name list to a
//      background worker (XDK) that runs the same ensure steps ahead of
//      the render — HDD seeks happen while the current frame draws.
//  Per the XDK docs: ReadFileScatter/XFileCache/physical sort keys are
//  DVD-only (GDFX) and do NOT apply to HDD/STFS; a plain seek+read over
//  an open file is the correct and sufficient primitive here.
// =====================================================================

bool PakLoader::ReadAt(u32 off, u32 size, u8* dst)
{
    if (!m_file || !dst) return false;
    if (size == 0) return true;
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    if (m_cs) EnterCriticalSection((CRITICAL_SECTION*)m_cs);   // recursive: safe under Ensure*
#endif
    const bool ok = (fseek((FILE*)m_file, (long)off, SEEK_SET) == 0) &&
                    (fread(dst, 1, size, (FILE*)m_file) == size);
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    if (m_cs) LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#endif
    return ok;
}

// Per-sound normalization after its blob lands in RAM. Same pipeline as
// the eager NormalizeSounds() passes 1/3/4, but per entry, plus: the raw
// pak format is respected (passthrough blobs like SL's mp3s stay
// unplayable instead of being submitted as garbage PCM).
void PakLoader::NormalizeOne(int idx)
{
    PakLoadedSound& s = m_sounds[idx];
    s.bits = 16;
    bool riffOk = false;
    if (s.data && s.dataSize >= 64) {
        SndRiffInfo ri;
        if (SndRiffParse(s.data, s.dataSize, ri)) {
            s.data      += ri.dataOff;
            s.dataSize   = ri.dataSize;
            s.sampleRate = ri.rate;
            s.channels   = ri.channels;
            s.bits       = ri.bits;
            riffOk = true;
            ++m_sndRiff;
        }
    }
    if (s.pakFmt != 0 && !riffOk) { s.format = 1; return; }   // passthrough (mp3)
    if (!s.data || s.dataSize == 0)  { s.format = 1; return; }

    if (s.bits == 8 || SndIsKnown8bit(s.name, s.dataSize)) {
        const u32 n = s.dataSize;
        u8* out = new u8[n * 2];
        if (!out) { s.format = 1; return; }
        for (u32 k = 0; k < n; ++k) {
            const int v = ((int)s.data[k] - 128) << 8;
            out[k * 2 + 0] = (u8)(v & 0xFF);
            out[k * 2 + 1] = (u8)((v >> 8) & 0xFF);
        }
        OwnedBuf* b = new OwnedBuf;
        b->data = out; b->next = m_owned; m_owned = b;
        s.data = out;
        s.dataSize = n * 2;
        s.bits = 16;
        ++m_snd8bit;
    } else if (s.bits != 16) {
        s.format = 1; return;
    } else if (m_swapAll) {
        SndSwap16InPlace(s.data, s.dataSize);
        ++m_sndSwapped;
    }
    s.format = 0;

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    // XDK "Audio Data and Endianness": the 360 XAudio2 mixes BE — same
    // final swap as the eager path (see NormalizeSounds pass 4).
    if (s.format == 0 && s.bits == 16 && s.data && s.dataSize >= 2)
        SndSwap16InPlace(s.data, s.dataSize);
#endif
}

// Choose mid-size 16-bit sounds for the load-time mini-vote.
void PakLoader::PickVoteCandidates()
{
    m_voteCount = 0;
    for (int i = 0; i < m_numSounds && m_voteCount < 8; ++i) {
        const PakLoadedSound& s = m_sounds[i];
        if (s.pakFmt != 0) continue;                          // PCM only
        if (SndIsLiteralNoise(s.name)) continue;              // static loops abstain
        if (SndIsKnown8bit(s.name, s.dataSize)) continue;
        if (s.dataSize < 100000 || s.dataSize > 2000000) continue;
        m_voteIdx[m_voteCount++] = i;
    }
}

void PakLoader::EnsureTextureLoaded(int idx)
{
    if (idx < 0 || idx >= m_numTextures) return;
    PakLoadedTexture& t = m_textures[idx];
    if (t.gpuReady) return;
    if (!t.dataSize || t.dataSize > 64u * 1024u * 1024u) { t.gpuReady = true; return; }
    u8* staging = new u8[t.dataSize];
    if (!staging) { t.gpuReady = true; return; }
    if (!ReadAt(t.fileOff, t.dataSize, staging)) {
        delete[] staging;
        t.gpuReady = true;                    // failed: mark done, texture stays 0
        return;
    }
    UploadTexture(idx, staging);
    delete[] staging;                         // UMA: it is inside the D3D texture now
}

void PakLoader::EnsureSoundLoaded(int idx)
{
    if (idx < 0 || idx >= m_numSounds) return;
    PakLoadedSound& s = m_sounds[idx];
    if (s.resident) return;
    if (!s.dataSize) { s.resident = true; s.format = 1; return; }
    u8* buf = new u8[s.dataSize];
    if (!buf) { s.resident = true; s.format = 1; return; }
    if (!ReadAt(s.fileOff, s.dataSize, buf)) {
        delete[] buf;
        s.resident = true; s.format = 1;
        return;
    }
    OwnedBuf* b = new OwnedBuf;
    b->data = buf; b->next = m_owned; m_owned = b;
    s.data = buf;
    NormalizeOne(idx);
    s.resident = true;
}

bool PakLoader::LoadStreaming(const char* pakPath, void* device)
{
    if (!pakPath || !device) return false;
    Unload();
    m_device = device;
    m_streaming = true;

    FILE* f = fopen(pakPath, "rb");
    if (!f) { m_streaming = false; return false; }
    m_file = f;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { Unload(); return false; }
    m_pakSize = (u32)sz;

    // ---- header ----
    u8 hdr[32];
    if (!ReadAt(0, 32, hdr) || LoadBE32(hdr) != 0x464E4146) { Unload(); return false; }
    const u32 numTex = LoadBE32(hdr + 8);
    const u32 numSnd = LoadBE32(hdr + 12);
    const u32 stringsOffset = LoadBE32(hdr + 24);
    const u32 dataOffset    = LoadBE32(hdr + 28);
    if (numTex > 100000 || numSnd > 100000 ||
        stringsOffset >= dataOffset || dataOffset > m_pakSize) { Unload(); return false; }

    // ---- resident region: header + tex table + snd table + name pool ----
    m_pakData = new u8[dataOffset];
    if (!m_pakData) { Unload(); return false; }
    if (!ReadAt(0, dataOffset, m_pakData)) { Unload(); return false; }

    m_numTextures = (int)numTex;
    m_numSounds = (int)numSnd;
    if (m_numTextures > 0) { m_textures = new PakLoadedTexture[m_numTextures]; memset(m_textures, 0, sizeof(PakLoadedTexture) * m_numTextures); }
    if (m_numSounds   > 0) { m_sounds   = new PakLoadedSound[m_numSounds];   memset(m_sounds,   0, sizeof(PakLoadedSound)   * m_numSounds);   }

    const u8* p = m_pakData + 32;
    for (int i = 0; i < m_numTextures; ++i) {
        const u32 nameOff  = LoadBE32(p); p += 4;
        const u32 dataOff  = LoadBE32(p); p += 4;
        const u32 dataSize = LoadBE32(p); p += 4;
        const u32 fmt      = LoadBE32(p); p += 4;
        const u32 origW    = LoadBE32(p); p += 4;
        const u32 origH    = LoadBE32(p); p += 4;
        const u32 alignW   = LoadBE32(p); p += 4;
        const u32 alignH   = LoadBE32(p); p += 4;
        p += 4; // mip unused
        const char* nameStr = (const char*)(m_pakData + nameOff);
        strncpy(m_textures[i].name, nameStr, 63);
        m_textures[i].name[63] = '\0';
        m_textures[i].origWidth = origW;
        m_textures[i].origHeight = origH;
        m_textures[i].alignedWidth = alignW;
        m_textures[i].alignedHeight = alignH;
        m_textures[i].format = fmt;
        m_textures[i].fileOff = dataOff;
        m_textures[i].dataSize = dataSize;
        m_textures[i].gpuReady = false;      // lazy: uploaded on first use
    }
    for (int i = 0; i < m_numSounds; ++i) {
        const u32 nameOff = LoadBE32(p); p += 4;
        const u32 dataOff = LoadBE32(p); p += 4;
        const u32 dataSize = LoadBE32(p); p += 4;
        const u32 fmt  = LoadBE32(p); p += 4;
        const u32 sr   = LoadBE32(p); p += 4;
        const u32 ch   = LoadBE32(p); p += 4;
        p += 4 * 2; // loop points unused
        const char* nameStr = (const char*)(m_pakData + nameOff);
        strncpy(m_sounds[i].name, nameStr, 63);
        m_sounds[i].name[63] = '\0';
        m_sounds[i].data = 0;                // lazy: read on first FindSound/Play
        m_sounds[i].dataSize = dataSize;
        m_sounds[i].format = 1;              // until loaded+normalized
        m_sounds[i].sampleRate = sr;
        m_sounds[i].channels = ch;
        m_sounds[i].bits = 16;
        m_sounds[i].fileOff = dataOff;
        m_sounds[i].resident = false;
        m_sounds[i].pakFmt = fmt;
    }

    // ---- pak-wide PCM byte-order mini-vote (same probe as the eager
    // path, but only a 4 KB window of a few mid-size sounds is read) ----
    PickVoteCandidates();
    int voteSwap = 0, voteNative = 0;
    for (int v = 0; v < m_voteCount; ++v) {
        const int i = m_voteIdx[v];
        u32 win = m_sounds[i].dataSize;
        if (win > 4096) win = 4096;
        u8* winBuf = new u8[win];
        if (ReadAt(m_sounds[i].fileOff, win, winBuf)) {
            const int r = SndPcmByteOrderVote(winBuf, win);
            if (r > 0) ++voteSwap;
            else if (r == 0) ++voteNative;
        }
        delete[] winBuf;
    }
    m_swapAll = (voteSwap > voteNative) && voteSwap > 0;

#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    const char* sndTarget = "BE(360)";
#else
    const char* sndTarget = "LE(PC)";
#endif
    printf("PakLoader: STREAMING '%s' (%.1f MB on disk, %d KB resident; %d tex lazy, %d snd lazy; vote %d/%d, target %s)\n",
           pakPath, (double)m_pakSize / (1024.0 * 1024.0),
           (int)(dataOffset / 1024), m_numTextures, m_numSounds,
           voteSwap, voteNative, sndTarget);

    // ---- background preload worker (XDK); PC falls back to synchronous ----
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    m_cs = new CRITICAL_SECTION;
    InitializeCriticalSection((CRITICAL_SECTION*)m_cs);
    m_wakeEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    m_qHead = m_qTail = 0;
    m_quit = false;
    m_thread = CreateThread(NULL, 0,
                            (LPTHREAD_START_ROUTINE)&PakLoader::PreloadThreadStatic,
                            this, 0, NULL);
#endif
    return true;
}

// XDK worker: waits on the wake event (or 50 ms), drains the ring of
// texture indices and runs the same ensure path the render thread would.
unsigned long PakLoader::PreloadThreadStatic(void* param)
{
    ((PakLoader*)param)->PreloadThreadRun();
    return 0;
}

void PakLoader::PreloadThreadRun()
{
    while (!m_quit) {
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
        WaitForSingleObject((HANDLE)m_wakeEvent, 50);
#endif
        for (;;) {
            int idx = -1;
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
            if (m_cs) EnterCriticalSection((CRITICAL_SECTION*)m_cs);
#endif
            if (m_qHead != m_qTail) {
                idx = (int)m_queue[m_qHead % 64];
                m_qHead++;
            }
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
            if (m_cs) LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
#endif
            if (idx < 0) break;
            EnsureTextureLoaded(idx);
        }
    }
}

void PakLoader::PreloadAsync(const char* const* texNames, int count)
{
    if (!m_streaming || !texNames || count <= 0) return;
#if defined(_XBOX) || defined(_XBOX360) || defined(_M_PPCBE)
    for (int n = 0; n < count; ++n) {
        if (!texNames[n]) continue;
        for (int i = 0; i < m_numTextures; ++i) {
            if (strcmp(m_textures[i].name, texNames[n]) == 0) {
                if (m_textures[i].gpuReady) break;         // already there
                if (m_cs) EnterCriticalSection((CRITICAL_SECTION*)m_cs);
                const int tail = m_qTail;
                if (tail - m_qHead < 64) { m_queue[tail % 64] = i; m_qTail = tail + 1; }
                if (m_cs) LeaveCriticalSection((CRITICAL_SECTION*)m_cs);
                if (m_wakeEvent) SetEvent((HANDLE)m_wakeEvent);
                break;
            }
        }
    }
#else
    for (int n = 0; n < count; ++n) {
        if (!texNames[n]) continue;
        for (int i = 0; i < m_numTextures; ++i) {
            if (strcmp(m_textures[i].name, texNames[n]) == 0) { EnsureTextureLoaded(i); break; }
        }
    }
#endif
}

int PakLoader::GetTextureCount() const { return m_numTextures; }
PakLoadedTexture* PakLoader::GetTexture(int idx) { if(idx<0||idx>=m_numTextures) return 0; return &m_textures[idx]; }
PakLoadedTexture* PakLoader::FindTexture(const char* name) {
    if(!name) return 0;
    for(int i=0;i<m_numTextures;++i) {
        if(strcmp(m_textures[i].name, name)==0) {
            if (m_streaming && !m_textures[i].gpuReady) EnsureTextureLoaded(i);  // v2.29
            return &m_textures[i];
        }
    }
    return 0;
}
int PakLoader::GetSoundCount() const { return m_numSounds; }
PakLoadedSound* PakLoader::GetSound(int idx) { if(idx<0||idx>=m_numSounds) return 0; return &m_sounds[idx]; }
PakLoadedSound* PakLoader::FindSound(const char* name) {
    if(!name) return 0;
    for(int i=0;i<m_numSounds;++i) {
        if(strcmp(m_sounds[i].name, name)==0) {
            if (m_streaming && !m_sounds[i].resident) EnsureSoundLoaded(i);      // v2.29
            return &m_sounds[i];
        }
    }
    return 0;
}

} // namespace fnaf
