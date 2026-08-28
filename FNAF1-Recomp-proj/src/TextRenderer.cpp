/**
 * Five Nights at Freddy's 1 -- Recompilation
 * TextRenderer.cpp: Xbox 360 stub - D3DX font placeholder
 */

#include "TextRenderer.h"
#include <cstdarg>
#include <cstring>
#include <cstdio>

#if defined(_XBOX) || defined(_XBOX360)
#include <xtl.h>
#include <d3dx9.h>
#else
#error "Only Xbox 360 target supported"
#endif

namespace fnaf {

TextRenderer::TextRenderer()
    : m_device(0)
    , m_font(0)
    , m_initialized(false)
{
    m_fmtBuf[0] = '\0';
}

TextRenderer::~TextRenderer()
{
    Shutdown();
}

bool TextRenderer::Init(void* device, i32 height, const char* faceName)
{
    if (m_initialized) return true;
    if (!device) return false;

    m_device = device;

#if defined(_XBOX) || defined(_XBOX360)
    // On Xbox 360, D3DX font creation is stubbed - just mark initialized
    // Real font rendering would use AtgFont or XUI, but for now use placeholder
    m_font = 0;
    m_initialized = true;
    return true;
#else
#error "Only Xbox 360 target supported"
#endif
}

void TextRenderer::Shutdown()
{
#if defined(_XBOX) || defined(_XBOX360)
    // No font to release in stub
#endif
    m_device = 0;
    m_font = 0;
    m_initialized = false;
}

void TextRenderer::DrawText(i32 x, i32 y, const char* text, u32 color)
{
    if (!m_initialized || !text || !text[0]) return;
#if defined(_XBOX) || defined(_XBOX360)
    // Stub - on Xbox, text rendering via D3DX is not available in this stub
    // In real XDK, would use AtgFont::DrawText or similar
    (void)x; (void)y; (void)color;
    // For now, also print to debug output
    // printf("[Xbox Text] %s\n", text);
    (void)text;
#else
#error "Only Xbox 360 target supported"
#endif
}

void TextRenderer::DrawTextf(i32 x, i32 y, u32 color, const char* fmt, ...)
{
    if (!m_initialized || !fmt) return;

    va_list args;
    va_start(args, fmt);
    vsnprintf(m_fmtBuf, sizeof(m_fmtBuf) - 1, fmt, args);
    m_fmtBuf[sizeof(m_fmtBuf) - 1] = '\0';
    va_end(args);
    DrawText(x, y, m_fmtBuf, color);
}

void TextRenderer::DrawTextCentered(i32 y, const char* text, u32 color, i32 screenWidth)
{
    if (!m_initialized || !text || !text[0]) return;

    i32 textLen = (i32)strlen(text);
    i32 approxWidth = textLen * 8;
    i32 x = (screenWidth - approxWidth) / 2;
    if (x < 0) x = 0;

    DrawText(x, y, text, color);
}

void TextRenderer::DrawTextCenteredXY(i32 cx, i32 cy, const char* text, u32 color)
{
    if (!m_initialized || !text || !text[0]) return;

    i32 textLen = (i32)strlen(text);
    i32 approxWidth = textLen * 8;
    i32 x = cx - approxWidth / 2;
    i32 y = cy - 12;

    DrawText(x, y, text, color);
}

bool TextRenderer::IsInitialized() const
{
    return m_initialized;
}

} // namespace fnaf