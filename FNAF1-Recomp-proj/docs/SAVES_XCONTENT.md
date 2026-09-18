# Saves and XContent on Xbox 360 — the canonical pattern (v2.39)

Goal: how to correctly write saves into a player profile per the XDK
documentation.

## The XDK canon

Sources (official XDK docs): `xbox-docs/md/XCreateContent.md`
(XContentCreate), `XContentCreateEx.md`, `XCloseContent.md`,
`xcontentflush.md`, `developing_with_xbox_360_unified_storage.md`
(white paper, ATG), `XCONTENT_DATA.md`, `XCONTENTFLAGs.md`,
`Kernel_File_Systems.md`.

The correct save-write cycle (profile storage):

1. **Device selection**: `XShowDeviceSelectorUI(dwUserIndex, XCONTENTTYPE_SAVEDGAME, flags, size, &DeviceID, &overlapped)` —
   must be called in overlapped form only and pumped to completion
   (`XHasOverlappedIoCompleted` + `XGetOverlappedResult`);
2. **Create/mount the container**:
   `XContentCreate(dwUserIndex, "drive", &contentData, XCONTENTFLAG_CREATEALWAYS, …)`
   with `contentData{ DeviceID, dwContentType = XCONTENTTYPE_SAVEDGAME,
   szDisplayName (what the user sees), szFileName (container name ≤ 40 ANSI) }`.
   Or `XContentCreateEx` with presize and cache (careful: presize makes the
   container fixed-size — it cannot grow);
3. **Write the file** with a plain `fopen("drive:\\my_file")` / `CreateFile`
   (the container hosts a full filesystem, many files allowed; the fopen name
   does NOT have to match the container's `szFileName`);
4. **Commit**: `XContentFlush("drive", NULL)` (buffer flush), then
   `XContentClose("drive", NULL)` — **must return success, otherwise the
   write does not count as written**.

Key constraints per the docs:

- `dwUserIndex` = the index of a LOCALLY SIGNED-IN gamer (0–3). Without a
  signed-in profile, `XContentCreate` returns **`ERROR_ACCESS_DENIED`**;
  the `XContentCreateEx` presize caps file growth; names inside the
  container are case-insensitive (FATX/STFS) — prefer lowercase;
  `szDisplayName` ≤ 128, `szFileName` ≤ 40 (ANSI), the logical root name
  (szRootName) is 1–12 chars and must not be in the reserved list
  (game:/d:/cache:/devkit:/e:). Overwriting an existing save is the
  normal flow (`XCONTENTFLAG_CREATEALWAYS`).

## Our choice (FNAF port)

- **Bound to the FIRST player — index 0** (`PickSignedInUserIndex`:
  `XUserGetSigninState(0) != XUSER_NOT_SIGNED_IN`). The original hangs its
  save and achievements on the first player's profile (the boot import
  round-tripped through that profile), so no 0..3 scan is needed: one
  subject — slot 0;
- With a profile signed in: `XContentCreateEx(0, "fnaf_save", …)` →
  `fopen("fnaf_save:\freddy" / "fnaf_ach.ini")` → Flush → Close, codes
  logged;
- Without a profile (RGH with no account): **local fallback**
  `game:\save_fallback\freddy` + `fnaf_ach.ini` — progress and achievements
  survive a game over (important: the folder deliberately does NOT overlap
  `game:\save\`, so boot never offers to import our own save).
- Achievements (`XUserWriteAchievements` in Achievements.cpp) are gated by
  the same rule: nobody signed in at slot 0 → the system write is skipped,
  the local INI remains the source of truth. `dwUserIndex = 0`.

Diagnostics: `XContent: user=N create=0x… disp=0x…`,
`XContent: flush=0x… close=0x…`. Expected codes on success — 0
(ERROR_SUCCESS).