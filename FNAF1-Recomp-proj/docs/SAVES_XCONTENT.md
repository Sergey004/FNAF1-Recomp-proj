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

## v2.50 — the device is picked once, up front

`Progress::PrimeStorage()` (called at boot before any message box) runs
`PickStorageDevice()` — formerly the selector half of `XContentMount`. Per the
XDK docs, `XShowDeviceSelectorUI` silently returns the only suitable device
when exactly one exists (Slim-era console with just the internal HDD — no MU
slots), so on that hardware nothing in the save path ever shows or even
initializes XAM's selector UI mid-flow: import, saves and achievements ride
the cached `g_saveDevice`. The remaining retry/cooldown logic only matters if
development devices with MUs appeared; the mount log now prints
`device=0x…` for evidence.

There is no documented "enumerate devices silently" API — the selector with
its single-device auto-answer is the supported route.

## v2.48 — the mounted-container leak (real-HW bug)

If the container-mounted `fopen` FAILED after a successful
`XContentCreateEx`, the old code dropped `s_usingXcontent` without closing,
so `StorageClose` skipped the unmount and the root stayed mounted for the
rest of the boot — every later `XContentCreateEx` then failed with 0x20
(`ERROR_SHARING_VIOLATION`) / 0xB7 (`ERROR_ALREADY_EXISTS`). Fixed inside
`StorageOpen`: on in-container fopen failure the root is flushed+closed
IMMEDIATELY (and the failing `GetLastError()` is logged), and a mount that
runs into those codes closes the stale root and retries once.
## v2.80 — one container per game module ("как миры Minecraft")

Pre-v2.80 all five modules wrote into ONE container (`freddy` under
`fnaf_save`, display "Five Nights at Freddy's 1 Save") — a single
dashboard tile held `freddy`, `freddy2`, `freddy3`, `fn4`, `sl` and the
two achievement INIs. v2.80 gives every module its OWN package, exactly
the Minecraft LCE world scheme (one world = one `XCONTENT_DATA` with its
own `szDisplayName` tile and unique `szFileName`):

| files | mount root | container | dashboard tile |
|---|---|---|---|
| `freddy`, `fnaf_ach.ini` | `fnaf_save` | `freddy` | Five Nights at Freddy's 1 Save |
| `freddy2`, `fnaf2_ach.ini` | `fnaf2_save` | `freddy2` | Five Nights at Freddy's 2 Save |
| `freddy3` | `fnaf3_save` | `freddy3` | Five Nights at Freddy's 3 Save |
| `fn4` | `fnaf4_save` | `fn4` | Five Nights at Freddy's 4 Save |
| `sl` | `fnsl_save` | `sl` | Sister Location Save |

Mechanics:
- Every save file name is module-unique, so `StorageOpen` resolves the
  container from the file name (`kSaveContainers` table) — no game-side
  call changed. The mount/unmount-per-operation pattern means two
  containers are never mounted at once; `StorageClose` unmounts exactly
  the root of the last mount (`s_mountedRoot`). Display names are capped
  at 31 WCHARs (the 360's 32-WCHAR limit incl. terminator), roots at 12
  chars. The thumbnail (game_icon.png) is set once per container.
- **Migration**: `Progress::MigrateLegacySave(file)` (called from each
  module's `Load` in AppModules.cpp before its progress read) copies a
  module's file out of the legacy container when the module container
  lacks it. The legacy container IS FNAF1's container (same `szFileName`),
  so F1 migrates by identity and its tile keeps the F1 progress; the
  legacy copies of F2/F3/F4/SL files are NOT deleted — the F1 tile doubles
  as a complete backup of the migration moment (delete it from the
  dashboard manually if you want; the modules never read it again).
- The local fallback (`game:\save_fallback\`) and the Live Safe build
  (`game:\save\`) stay flat — their files are already module-unique and
  neither is dashboard-visible.
- Future (not built): Minecraft-style per-game save SLOTS ("worlds") ride
  the same pattern — enumerate with `XContentCreateEnumerator` +
  `XEnumerate` over `XCONTENTTYPE_SAVEDGAME`, mint a fresh `szFileName`
  per new world (Minecraft uses a creation timestamp), reuse the stored
  `XCONTENT_DATA` for updates, and rename = create-new + copy + delete-old
  (there is no XContentRename). The world list lives in the content
  enumeration, never in the profile.
