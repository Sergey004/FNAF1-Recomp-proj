/**
 * DevIni.h — v2.111: the dev.ini parser (the per-console dev flags).
 *
 * The file sits next to the XEX (game:\dev.ini) and is read once at boot:
 *
 *   console=1      on-screen debug console (Usage/Tick log)
 *   devmenu=1      the DEV menu (Start+B toggles it anyway)
 *   god=1          god mode (the animatronics never kill)
 *   analog=1       analog doors (LT/RT drive the door position)
 *   holdlights=1   lights as a hold button (LB/RB held)
 *   browser=1      sprite browser (LB+RB in the menu)
 *   tuner=1        perspective tuner (L3+R3)
 *
 * EVERY flag has its code default, and the config only ever CHANGES what
 * it names: a key that is not in the file — or whose value is not
 * understood — leaves the flag at its default. v2.82's parser could only
 * switch flags ON (`if (!on) continue;` and no notion of a default), so a
 * missing or misspelled key silently left the feature off instead of
 * falling back to the default.
 *
 * The values are 1/0, true/false, yes/no, on/off (case-insensitive).
 * VS2010/C++03.
 */
#ifndef FNAF_DEVINI_H
#define FNAF_DEVINI_H

#include <string.h>

struct DevIniFlags {
    // -1 = not set (keep the code default), 0 = off, 1 = on
    int console, devmenu, god, analog, holdlights, browser, tuner;
    DevIniFlags() {
        console = devmenu = god = analog = holdlights = browser = tuner = -1;
    }
    bool Any() const {
        return console >= 0 || devmenu >= 0 || god >= 0 || analog >= 0 ||
               holdlights >= 0 || browser >= 0 || tuner >= 0;
    }
};

static bool DevStrEqNoCase(const char* a, const char* b) {
    for (;; ++a, ++b) {
        char ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca = (char)(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = (char)(cb - 'A' + 'a');
        if (ca != cb) return false;
        if (!ca) return true;
    }
}

// 1 = on, 0 = off, -1 = not understood (the caller keeps the default)
static int DevParseBool(const char* v) {
    while (*v == ' ' || *v == '\t') ++v;
    if (!*v) return -1;
    if (DevStrEqNoCase(v, "1")   || DevStrEqNoCase(v, "true")  ||
        DevStrEqNoCase(v, "yes") || DevStrEqNoCase(v, "on")    ||
        DevStrEqNoCase(v, "enable")  || DevStrEqNoCase(v, "enabled"))  return 1;
    if (DevStrEqNoCase(v, "0")   || DevStrEqNoCase(v, "false") ||
        DevStrEqNoCase(v, "no")  || DevStrEqNoCase(v, "off")   ||
        DevStrEqNoCase(v, "disable") || DevStrEqNoCase(v, "disabled")) return 0;
    return -1;
}

static void DevSetFlag(DevIniFlags& f, const char* key, int value) {
    if      (DevStrEqNoCase(key, "console"))    f.console    = value;
    else if (DevStrEqNoCase(key, "devmenu"))    f.devmenu    = value;
    else if (DevStrEqNoCase(key, "god"))        f.god        = value;
    else if (DevStrEqNoCase(key, "analog"))     f.analog     = value;
    else if (DevStrEqNoCase(key, "holdlights")) f.holdlights = value;
    else if (DevStrEqNoCase(key, "browser"))    f.browser    = value;
    else if (DevStrEqNoCase(key, "tuner"))      f.tuner      = value;
}

// Parses ONE line (the line is trimmed in place). Returns true when a
// known key with an understood value was applied; false when the line is
// a comment, a section header, malformed, or names an unknown key/value
// — in every one of those cases the flags keep their defaults.
static bool DevIniParseLine(char* line, DevIniFlags& out) {
    char* s = line;
    while (*s == ' ' || *s == '\t') ++s;
    // cut an inline comment ('#' or ';')
    for (char* c = s; *c; ++c) {
        if (*c == '#' || *c == ';') { *c = '\0'; break; }
    }
    // trim the end of the line
    char* e = s + strlen(s);
    while (e > s && (e[-1] == ' ' || e[-1] == '\t' ||
                     e[-1] == '\n' || e[-1] == '\r')) { --e; *e = '\0'; }
    if (!s[0]) return false;
    if (s[0] == '[') return false;          // a section header — the file is flat
    char* eq = strchr(s, '=');
    if (!eq) return false;                  // no value — keep the default
    *eq = '\0';
    char* key = s;
    char* val = eq + 1;
    // trim BOTH ends of the key ("console = 1" must work)
    while (*key == ' ' || *key == '\t') ++key;
    char* ke = key + strlen(key);
    while (ke > key && (ke[-1] == ' ' || ke[-1] == '\t')) { --ke; *ke = '\0'; }
    while (*val == ' ' || *val == '\t') ++val;
    if (!key[0]) return false;
    const int v = DevParseBool(val);
    if (v < 0) return false;                // an unknown value — keep the default
    DevSetFlag(out, key, v);
    return true;
}

#endif // FNAF_DEVINI_H
