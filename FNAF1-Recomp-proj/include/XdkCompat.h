/**
 * Five Nights at Freddy's 1 — Recompilation
 * XdkCompat.h: tiny portability shims for the Xbox 360 XDK toolset.
 *
 * The XDK CRT (VS2010-era, MSVC 10.0) predates C99: there is no snprintf
 * (only _snprintf, which does not null-terminate on truncation).
 * vsnprintf DOES exist in the XDK stdio.h and always terminates within
 * its count, so it is used as the base for a safe snprintf replacement.
 *
 * Include this header in any translation unit that formats text.
 */

#pragma once

// Silence the XDK CRT's _CRT_INSECURE_DEPRECATE notes (C4996) for the
// legacy string/stdio functions this project deliberately uses.
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdarg.h>

namespace fnaf {

// C99-style snprintf built on the XDK CRT.
//  - writes at most `count` bytes including the terminator
//  - ALWAYS leaves dst a valid null-terminated string, even on truncation
//  - returns the number of characters written (excluding the terminator),
//    or count-1 if the output had to be truncated
//
// CRT flavor notes:
//  - VS2005..2013 (incl. the XDK): vsnprintf returns -1 on truncation and
//    may fill the buffer completely without terminating (like _vsnprintf).
//  - C99-conformant CRTs (VS2015+, glibc): return the would-be length and
//    always terminate within count bytes.
// Both are handled below; a format error (-1 without output) still leaves
// the pre-set empty string in dst.
inline int Snprintf(char* dst, unsigned int count, const char* fmt, ...)
{
    if (!dst || count == 0) return 0;
    dst[0] = '\0';
    va_list args;
    va_start(args, fmt);
    const int n = vsnprintf(dst, count, fmt, args);
    va_end(args);
    if (n < 0 || n >= (int)count) {
        dst[count - 1] = '\0';        // force termination inside the buffer
        return (int)(count - 1);
    }
    dst[n] = '\0';
    return n;
}

} // namespace fnaf
