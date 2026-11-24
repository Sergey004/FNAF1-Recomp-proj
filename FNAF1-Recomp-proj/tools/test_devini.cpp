/**
 * tools/test_devini.cpp — v2.111: the dev.ini parser test.
 *
 * The parser lives in include/DevIni.h (header-only) so it can be checked
 * here on the PC — the Xbox build only pulls it into main.cpp.
 *
 * The rule under test: the config only ever CHANGES what it names. A key
 * that is missing, malformed or carries an unknown value leaves the flag
 * at its default (-1 = "keep the default"), it never silently turns the
 * feature off.
 *
 * Build + run:
 *   g++ -std=gnu++11 -Iinclude tools/test_devini.cpp -o /tmp/test_devini && /tmp/test_devini
 */
#include "DevIni.h"

#include <cstdio>
#include <cstring>

static int g_fail = 0;
static int g_checks = 0;

static void Check(const char* what, int got, int want) {
    ++g_checks;
    if (got != want) {
        ++g_fail;
        printf("FAIL %-52s got %d want %d\n", what, got, want);
    }
}

// parse a whole file body into the flags
static void Parse(const char* text, DevIniFlags& f) {
    char buf[1024];
    const char* p = text;
    while (*p) {
        const char* nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        if (len >= sizeof(buf)) len = sizeof(buf) - 1;
        memcpy(buf, p, len);
        buf[len] = '\0';
        DevIniParseLine(buf, f);
        if (!nl) break;
        p = nl + 1;
    }
}

int main() {
    // 1. an empty file leaves every flag at the default
    {
        DevIniFlags f;
        Parse("", f);
        Check("empty: console", f.console, -1);
        Check("empty: god", f.god, -1);
        Check("empty: Any()", f.Any() ? 1 : 0, 0);
    }

    // 2. the plain form
    {
        DevIniFlags f;
        Parse("console=1\ndevmenu=1\ngod=1\nanalog=1\nholdlights=1\nbrowser=1\ntuner=1\n", f);
        Check("plain: console", f.console, 1);
        Check("plain: devmenu", f.devmenu, 1);
        Check("plain: god", f.god, 1);
        Check("plain: analog", f.analog, 1);
        Check("plain: holdlights", f.holdlights, 1);
        Check("plain: browser", f.browser, 1);
        Check("plain: tuner", f.tuner, 1);
    }

    // 3. an explicit 0 switches the flag OFF (this is what v2.82 could
    //    not do — it only ever switched flags on)
    {
        DevIniFlags f;
        Parse("console=0\ngod=off\nbrowser=false\ntuner=no\n", f);
        Check("explicit off: console", f.console, 0);
        Check("explicit off: god", f.god, 0);
        Check("explicit off: browser", f.browser, 0);
        Check("explicit off: tuner", f.tuner, 0);
        Check("explicit off: devmenu untouched", f.devmenu, -1);
    }

    // 4. the word forms
    {
        DevIniFlags f;
        Parse("console=true\ndevmenu=YES\ngod=on\nanalog=Enabled\n", f);
        Check("words on: console", f.console, 1);
        Check("words on: devmenu", f.devmenu, 1);
        Check("words on: god", f.god, 1);
        Check("words on: analog", f.analog, 1);
    }

    // 5. the key case does not matter
    {
        DevIniFlags f;
        Parse("Console=1\nGOD=1\nHoLdLiGhTs=1\n", f);
        Check("case: console", f.console, 1);
        Check("case: god", f.god, 1);
        Check("case: holdlights", f.holdlights, 1);
    }

    // 6. spaces around the '=' and around the value (v2.82 failed here:
    //    it trimmed the line end but NOT the key's tail)
    {
        DevIniFlags f;
        Parse("  console  =  1  \n\tgod\t=\ttrue\t\n", f);
        Check("spaces: console", f.console, 1);
        Check("spaces: god", f.god, 1);
    }

    // 7. comments: whole lines and inline
    {
        DevIniFlags f;
        Parse("# a comment\n; another one\nconsole=1 ; turn the console on\n"
              "god=1 # god mode\n", f);
        Check("comments: console", f.console, 1);
        Check("comments: god", f.god, 1);
    }

    // 8. the malformed lines keep the default (they must NOT turn the
    //    flag off)
    {
        DevIniFlags f;
        Parse("console\nconsole=\nconsole=maybe\n=1\n[section]\n", f);
        Check("malformed: console", f.console, -1);
        Check("malformed: Any()", f.Any() ? 1 : 0, 0);
    }

    // 9. an unknown key is ignored, the known ones still apply
    {
        DevIniFlags f;
        Parse("nosuchkey=1\nconsole=1\n", f);
        Check("unknown key: console", f.console, 1);
    }

    // 10. CRLF line endings
    {
        DevIniFlags f;
        Parse("console=1\r\ngod=1\r\n", f);
        Check("crlf: console", f.console, 1);
        Check("crlf: god", f.god, 1);
    }

    // 11. the last line of a file without a trailing newline
    {
        DevIniFlags f;
        Parse("console=1\ngod=1", f);
        Check("no eol: console", f.console, 1);
        Check("no eol: god", f.god, 1);
    }

    // 12. the same key twice — the last one wins
    {
        DevIniFlags f;
        Parse("console=1\nconsole=0\n", f);
        Check("twice: console", f.console, 0);
    }

    // 13. the flags are independent of each other
    {
        DevIniFlags f;
        Parse("god=1\n", f);
        Check("only god: god", f.god, 1);
        Check("only god: console", f.console, -1);
        Check("only god: devmenu", f.devmenu, -1);
        Check("only god: analog", f.analog, -1);
        Check("only god: holdlights", f.holdlights, -1);
        Check("only god: browser", f.browser, -1);
        Check("only god: tuner", f.tuner, -1);
    }

    printf("%s: %d checks, %d failures\n", g_fail ? "FAILED" : "OK", g_checks, g_fail);
    return g_fail ? 1 : 0;
}
