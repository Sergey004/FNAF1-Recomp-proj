/**
 * Five Nights at Freddy's 1 -- Recompilation
 * DebugConsole.cpp: Simple framebuffer-style console overlay
 */

#include "DebugConsole.h"
#include "TextRenderer.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace fnaf {

DebugConsole::DebugConsole()
    : m_renderer(nullptr)
    , m_maxLines(128)
    , m_lineHeight(20)
    , m_lines(nullptr)
    , m_lineCount(0)
    , m_writeIndex(0)
{
}

DebugConsole::~DebugConsole()
{
    Clear();
    if (m_lines) {
        for (i32 i = 0; i < m_maxLines; ++i) {
            if (m_lines[i]) {
                delete[] m_lines[i];
            }
        }
        delete[] m_lines;
        m_lines = nullptr;
    }
}

void DebugConsole::Init(TextRenderer* renderer, i32 maxLines, i32 lineHeight)
{
    m_renderer = renderer;
    m_maxLines = maxLines;
    m_lineHeight = lineHeight;

    m_lines = new char*[m_maxLines];
    for (i32 i = 0; i < m_maxLines; ++i) {
        m_lines[i] = new char[512];
        m_lines[i][0] = '\0';
    }
    m_lineCount = 0;
    m_writeIndex = 0;
}

void DebugConsole::Clear()
{
    if (!m_lines) return;
    for (i32 i = 0; i < m_maxLines; ++i) {
        m_lines[i][0] = '\0';
    }
    m_lineCount = 0;
    m_writeIndex = 0;
}

void DebugConsole::Print(const char* fmt, ...)
{
    if (!m_renderer || !fmt) return;

    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf)-1, fmt, args);
    va_end(args);
    buf[sizeof(buf)-1] = '\0';

    PushLine(buf);
}

void DebugConsole::PushLine(const char* line)
{
    if (!m_lines) return;
    strncpy(m_lines[m_writeIndex], line, 511);
    m_lines[m_writeIndex][511] = '\0';

    m_writeIndex = (m_writeIndex + 1) % m_maxLines;
    if (m_lineCount < m_maxLines) m_lineCount++;
}

void DebugConsole::Render(i32 screenW, i32 screenH)
{
    if (!m_renderer || m_lineCount == 0) return;

    const u32 col = TextColor::GRAY;
    i32 yStart = screenH - m_lineHeight * m_lineCount - 10;
    if (yStart < 10) yStart = 10;

    i32 idx = (m_writeIndex - m_lineCount + m_maxLines) % m_maxLines;
    for (i32 i = 0; i < m_lineCount; ++i) {
        const char* line = m_lines[idx];
        if (line && line[0]) {
            m_renderer->DrawText(20, yStart + i * m_lineHeight, line, col);
        }
        idx = (idx + 1) % m_maxLines;
    }
}

} // namespace fnaf
