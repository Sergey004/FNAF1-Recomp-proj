# EXE version verdict & build verification (v2.7.4)

## 1. Verdict: the source exe is the FULL Steam game (user was right)

Earlier notes (v2.6/v2.7 era) wrongly called the exe "the FNaF1 demo".
That verdict is corrected in v2.7.4. The exe uploaded by the user is the
purchased Steam release, and the demo-flavoured strings inside it are
leftovers of the shared Clickteam MFA project, not evidence of a demo
build. Hard evidence from the CCN metadata + event tables:

1. **`targetFilename`**: `C:\Users\Scott\Desktop\sdk\tools\ContentBuilder\
   content\windows_content\FiveNightsatFreddys.exe` — **SteamPipe
   ContentBuilder** output path. The demo was never shipped through
   Steam's ContentBuilder pipeline.
2. **`editorFilename`**: `C:\Users\Scott\Desktop\Five Nights\
   FiveNights-55.mfa` — Scott's master project #55; demo and full game
   were built from the same project, which is exactly why demo objects
   remain in the data.
3. **The `DEMO?` counter (object 31) is never set anywhere** — a scan of
   all 17 frames finds ZERO actions writing to it, so it stays 0.
4. **Title frame**: the 'demo' watermark (obj 32 / img_572) is shown only
   under `IF counter DEMO? == 1` (group 62). With the counter pinned at 0
   the watermark can never appear — dead data in this build.
5. **'next day' frame**: the route to the 'end of demo' frame is also
   gated on `DEMO? == 1`; the `DEMO? == 0` branch (full-game progression)
   always runs. The 'end of demo' frame (frame 16) is unreachable.
6. **Full-game content present**: night cards for all 7 nights
   (453/454/472/473/474/446/538), 5th-night paycheck img_210, overtime
   img_522, pink slip img_523, 'custom night' object on the title. The
   2-night demo has none of these wired.
7. **Version string img_588 "v 1.132"** on the title — the shipped PC
   version marker, plus PE build date 2015-05-25 (Steam patch era).

Runtime behavior of the recomp is unchanged by this correction: the real
full game never draws the watermark, and neither do we.

## 2. Why "for VS it's as if the files didn't change"

The minipatch zips contain relative paths (`src/...`, `include/...`,
`docs/...`). They must be extracted INTO the inner project folder:

```
C:\Users\123\Desktop\FNAF1-Recomp-proj\FNAF1-Recomp-proj\
```

Extracting one level higher silently creates
`...\FNAF1-Recomp-proj\src\` next to the real project — VS keeps using
the old files. Windows "Extract All" also likes to add a nested folder
named after the archive; check the paths it offers before extracting.

After extracting: **Build -> Rebuild Solution** (Перестроить решение),
not just F5, so headers (.h) changes are guaranteed to recompile.

## 3. v2.7.4 verification markers

Three independent markers, all added in v2.7.4:

1. **Source markers** — open the file in VS and check the top comments:
   - `src/main.cpp` contains `v2.7.4: FIRST line of the log`
   - `src/GameRender.cpp` contains `v2.7.4 VERDICT CORRECTION`
   - `src/PakLoader.cpp` contains `v2.7.4: name the normalization target`
2. **VS Output window** — the FIRST log line at boot must be:
   ```
   === FNAF1-Recomp v2.7.4 built <date> <time> ===
   ```
   followed by `PakLoader: sounds normalized (..., target BE(360))`.
   `BE(360)` proves the v2.7 console byte-order pass is compiled in.
3. **On-screen debug console** (bottom-left overlay) shows the same
   version banner + the audio probe line.

`APPLY_PATCH.bat` in the minipatch zip checks markers 1 automatically.

## 4. Audio byte-order probe (noise diagnosis)

At boot, after the `Pak sounds:` line, the log prints the first 8 payload
bytes of the two first bank sounds. Reference table (computed from the
52 reference WAV dumps of THIS exe):

| sound               | correct v2.7.x build (BE for 360 XAudio2)     | pre-v2.7 build (LE -> noise)                  |
|---------------------|-----------------------------------------------|-----------------------------------------------|
| ColdPresc B         | `FF 86 00 8D FF 76 00 AA`                     | `86 FF 8D 00 76 FF AA 00`                     |
| BallastHumMedium2   | `FE 86 00 8F 01 EC 01 64`                     | `86 FE 8F 00 EC 01 64 01`                     |

- Log matches the LEFT column -> the whole audio chain (RIFF peel ->
  pak-wide vote -> LE normalize -> 8-bit expand -> BE for 360) ran; if
  noise is still heard in that case, report the exact `Snd[...]` line.
- Log matches the RIGHT column (or the `Snd[...]` lines are absent) ->
  the running XEX still contains the old PakLoader; the patch did not
  land (see section 2).
