/**
 * Five Nights at Freddy's 1 -- Recompilation
 * DebugConsole.h: Simple framebuffer-style console overlay
 *
 * Uses existing TextRenderer for drawing, but provides ATG-like
 * line buffer, scrolling and printf-style output.
 */

#ifndef FNAF_DEBUG_CONSOLE_H
#define FNAF_DEBUG_CONSOLE_H

#include "Types.h"

namespace fnaf {

class TextRenderer;

class DebugConsole {
public:
    DebugConsole();
    ~DebugConsole();

    void Init(TextRenderer* renderer, i32 maxLines = 128, i32 lineHeight = 20);
    void Clear();
    void Print(const char* fmt, ...);
    void Render(i32 screenW, i32 screenH);

private:
    TextRenderer* m_renderer;
    i32 m_maxLines;
    i32 m_lineHeight;
    char** m_lines;
    i32 m_lineCount;
    i32 m_writeIndex;

    void PushLine(const char* line);
};

} // namespace fnaf

#endif // FNAF_DEBUG_CONSOLE_H
