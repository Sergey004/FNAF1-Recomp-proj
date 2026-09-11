# Changelog — FNAF1 Recomp (Xbox 360)

Notes on what's done and what's left. Versions match the code comment tags
(`v2.8`, `v2.14`, `v2.15`, `v2.16`) and the historical notes.

Заметки «что у нас уже есть» и «что осталось». Версии соответствуют тегам в
комментариях кода (`v2.8`, `v2.14`, `v2.15`, `v2.16`) и историческим заметкам.

---

## v2.24 — animation speeds pinned to the PC dump / скорости анимаций выверены по PC-дампу

### English
- Confirmed the animation formula `fps = speed × 0.6` (÷60) against the original PC `application.json` (`frameRate: 60`), cross-checked with FNaF64 (which uses the same `speed_fps`). Fixed the approximations that were guessing instead of reading the dump:
  - **Desk fan** ("Active 6" anim 0, 3 frames): was a guessed ~8 FPS; now speed 99 = 59.4 FPS.
  - **REC light** ("Active 2" anim 0, `[7,5]`) and **cam-map blink** ("Active 9" anim 0, `[164,145]`): were ~1.25 Hz; now `speed 2` = 1.2 FPS via `CfAnimFrame`.
  - **Foxy sprint** ("Active 3" anim 51, 33 frames, speed 65 = 39 FPS): was stretched linearly over the 1.67 s run; now plays at 39 FPS and holds the last frame (~0.85 s in), as the original freeze.
- Re-verified (already correct, no change): jumpscares (Freddy 65@50×31, Foxy 52@50×25, Bonnie 35@75×11, Chica 44@99×16), doors (50→30 FPS, 16 frames), IT'S ME (75→45 FPS, 4 frames), static frames.
- Static kept at **24 FPS** (deliberate user tune; dump is speed 99 = 59.4 FPS).

### Русский
- Подтверждена формула `fps = speed × 0.6` (÷60) по оригинальному PC-дампу `application.json` (`frameRate: 60`), перекрёстно — с FNaF64 (там та же `speed_fps`). Убраны приближения, которые были «на глаз», а не из дампа:
  - **Вентилятор** («Active 6» anim 0, 3 кадра): было ~8 FPS наугад; теперь speed 99 = 59.4 FPS.
  - **REC** («Active 2» anim 0, `[7,5]`) и **blink точки на карте** («Active 9» anim 0, `[164,145]`): было ~1.25 Гц; теперь `speed 2` = 1.2 FPS через `CfAnimFrame`.
  - **Foxy sprint** («Active 3» anim 51, 33 кадра, speed 65 = 39 FPS): было линейно растянуто на 1.67 с бега; теперь 39 FPS с удержанием последнего кадра (~0.85 с), как замирание в оригинале.
- Перепроверено (уже верно, без изменений): скримеры (Freddy 65@50×31, Foxy 52@50×25, Bonnie 35@75×11, Chica 44@99×16), двери (50→30 FPS, 16 кадров), IT'S ME (75→45 FPS, 4 кадра), кадры static.
- Static оставлен на **24 FPS** (осознанный тюн пользователя; в дампе speed 99 = 59.4 FPS).

---

## v2.23 — original lives/save system fix / правка жизней и сохранений как в оригинале

### English
- Removed the non-1:1 "lives" system (the original FNAF1 has none). A jumpscare no longer decrements lives or resets progress; death just returns to the title and Continue retries the same (unlocked) night.
- Fixed a progression bug: the night-5/6/7 stars (`beat5/beat6/beat7`) were also awarded on GAME OVER — dying on night 5/6/7 incorrectly unlocked the star. Now they are gated on `GAME_STATE_NIGHT_COMPLETE`.
- Save storage:
  - The save file is the original extension-less **`freddy`** (INI content) in both builds. The Live Safe build now reads/writes it at **`game:\save\freddy`** (absolute, next to the .xex) instead of the relative `save\` folder; the system build keeps the XContent content `freddy`.
  - `lives` removed from the `[freddy]` INI and from `GameProgress`.
- **Save import**: at boot the system build checks `game:\freddy` / `game:\save\freddy` for a valid loose `[freddy]` INI; when found it asks via an Xbox message box ("Do you want to import the save found in the game folder?"); on confirm it imports into the XContent save (`fnaf_save:\freddy`), shows "Import complete. Please restart the game.", then closes the game. No-op in Live Safe.

### Русский
- Убрана не-1:1 система «жизней» (в оригинале FNAF1 их нет). Скример больше не декрементит жизни и не сбрасывает прогресс; смерть просто возвращает на титул, Continue стартует ту же (открытую) ночь.
- Починен баг прогресса: звёзды ночей 5/6/7 (`beat5/beat6/beat7`) выдавались и при GAME OVER — смерть на 5/6/7 ошибочно открывала звезду. Теперь они гейтятся на `GAME_STATE_NIGHT_COMPLETE`.
- Сохранения:
  - Файл сохранения — оригинальный **`freddy`** без расширения (INI-контент) в обеих сборках. Live Safe-сборка теперь пишет/читает его в **`game:\save\freddy`** (абсолютный путь рядом с .xex) вместо относительной папки `save\`; системная сборка сохраняет XContent-контент `freddy`.
  - `lives` убраны из INI `[freddy]` и из `GameProgress`.
- **Импорт сейва**: при старте системная сборка проверяет `game:\freddy` / `game:\save\freddy` на валидный `[freddy]`-INI; если найден — спрашивает через системное окно («Do you want to import the save found in the game folder?»), по подтверждению импортирует в XContent (`fnaf_save:\freddy`), показывает «Import complete. Please restart the game.» и закрывает игру. В Live Safe — no-op.

---

## v2.22 — Golden Freddy, camera audio, doors & remaining audio

### English

**Doors + animation timer**
- Restored the open/close slide in normal (non-analog) mode. `DoorSystem::ToggleDoor`/`SetDoor` flip the logical `m_doorClosed` instantly (the AI reads it), while the visual position `m_doorAmount` is driven by a new `DoorSystem::Tick(dt)`, ticked from the main loop via `Game::TickDoors(1/60)`. Slide = 16 frames @ 30 FPS (~0.53 s), as in the original.
- Extracted the Clickteam animation timer into `include/CfAnimTimer.h` (`CfFramePeriod` / `CfAnimFrame` / `struct CfAnimTimer`), formula `fps = speed × 0.6` (`period = (100/speed)/60`). Static, wipe, flash, raise and the jumpscares now use it — `GameRender.cpp` no longer duplicates the pattern.

**Camera audio (1:1)**
- `onCameraChange(cam, reason)` now receives a `CAM_REASON_*` reason:
  - monitor up (`UP`) → `CAMERA_VIDEO_LOA` whir (`Snd::CAMERA_SWITCH`) + start `static`;
  - camera switch (`SWITCH`) → `blip3` (`Snd::BLIP`), static is not restarted
    (previously the whir played on EVERY switch — the "extra sound");
  - monitor down (`DOWN`) → `put down` (`Snd::PUT_DOWN`) + stop `static`.
- Removed the forced `onCameraChange(CAM_OFF)` from power-out (it would play "put down" on every blackout; `OnPowerOut()` already stops the static).

**Golden Freddy ("yellow bear")**
- Full sequence in `main.cpp` (`TickGoldenFreddy`): 1/100 poster roll on monitor drop → giggle #38 (`Laugh_Giggle_Girl_1` = `Snd::FREDDY_LAUGH_LONG`) on CAM 2B → appears in the office (img 573 @660,478) on monitor drop → ~5 s to raise the monitor, or he kills you.
- The kill reuses the jumpscare state with `ANIM_COUNT` as sentinel (`Game::DebugTriggerGoldenFreddy`): `RenderJumpscare` draws img 571 full-screen, `GetJumpscareDurationSec` gives ~2.5 s, `OnJumpscare` guards the name array.

**Remaining audio triggers**
- Kitchen oven `OVEN_DRAW` (ch10) — Chica in the kitchen, every ~80 s (groups 245-250).
- Camcorder `TAPE_EJECT` (ch6) on monitor-up (group 144).
- Power-out: `AMBIENCE2` (drone, group 271/286) + "Active 2" `COMPUTER_DIG` and `garble1/2/3`, cycling with Freddy's face flashes (`Game::GetPowerOutFaceState`).
- Freddy's nose honk `PARTY_FAVOR` — Y in the office (group 349).

**Remaining approximations**
- `robotvoice` (ch21) / `EerieAmbience` (ch18) still use a room-based heuristic in `TickAudioMixer`, not the exact `vol zone` (objInfo 136) overlap from the dump.
- The "creepy start/end" frames are shortened to ~2.5 s (the original "creepy start" lingers ~200 s before crashing).

### Русский

**Двери + таймер анимаций**
- **Анимация закрытия/открытия восстановлена** в обычном (не аналоговом) режиме.
  `DoorSystem::ToggleDoor`/`SetDoor` мгновенно переключают логический `m_doorClosed`
  (его читает AI), а визуальную позицию `m_doorAmount` двигает новый
  `DoorSystem::Tick(dt)`, который тикается через `Game::TickDoors(1/60)` из
  главного цикла. Слайд — 16 кадров @ 30 FPS (~0.53 с), как в оригинале.
- Вынесен «таймер анимаций Clickteam» в `include/CfAnimTimer.h`
  (`CfFramePeriod` / `CfAnimFrame` / `struct CfAnimTimer`), формула
  `fps = speed × 0.6` (`период = (100/speed)/60`). Static, wipe, flash, raise и
  скримеры переведены на него — `GameRender.cpp` больше не дублирует паттерн.

**Звук камеры (1:1)**
- `onCameraChange(cam, reason)` теперь получает причину `CAM_REASON_*`:
  - подъём планшета (`UP`) → визг `CAMERA_VIDEO_LOA` (`Snd::CAMERA_SWITCH`) + запуск `static`;
  - переключение камеры (`SWITCH`) → `blip3` (`Snd::BLIP`), static не перезапускается
    (раньше на КАЖДОЕ переключение играл визг — тот самый «лишний звук»);
  - опускание (`DOWN`) → `put down` (`Snd::PUT_DOWN`) + стоп `static`.
- Убран принудительный `onCameraChange(CAM_OFF)` из power-out (он бы играл «put down»
  на каждом отключении света; `OnPowerOut()` и так глушит static).

**Golden Freddy («yellow bear»)**
- Полный цикл в `main.cpp` (`TickGoldenFreddy`): 1/100 ролл постера при опускании
  монитора → гига #38 (`Laugh_Giggle_Girl_1` = `Snd::FREDDY_LAUGH_LONG`) при взгляде
  на CAM 2B → появление в офисе (img 573 @660,478) при опускании планшета → ~5 с
  поднять планшет, иначе убийство.
- Убийство через джампскейр-состояние с sentinel `ANIM_COUNT`
  (`Game::DebugTriggerGoldenFreddy`): `RenderJumpscare` рисует img 571 полноэкран,
  `GetJumpscareDurationSec` даёт ~2.5 с, `OnJumpscare` не выходит за границы имён.

**Оставшиеся аудио-триггеры**
- Кухонная духовка `OVEN_DRAW` (ch10) — Chica в кухне, каждые ~80 с (groups 245-250).
- Camcorder `TAPE_EJECT` (ch6) при подъёме планшета (group 144).
- Power-out: `AMBIENCE2` (гул, group 271/286) + «Active 2» `COMPUTER_DIG` и
  `garble1/2/3`, циклирующиеся с вспышками лица Freddy (`Game::GetPowerOutFaceState`).
- Honk носа `PARTY_FAVOR` — Y в офисе (group 349).

**Оставшиеся приближения**
- `robotvoice` (ch21) / `EerieAmbience` (ch18) — по-прежнему эвристика по комнатам
  в `TickAudioMixer`, а не точная зона `vol zone` (objInfo 136) из дампа.
- Кадры «creepy start/end» укорочены до ~2.5 с (в оригинале «creepy start» висит
  ~200 с до краша).

---

## v2.21 — analog doors (DEV test feature) / аналоговые двери (DEV-тест)

### English
- `DoorSystem::SetDoorAmount/GetDoorAmount` — continuous door position 0..1, logical "closed" (AI) = `amount >= 0.5`. DEV toggle `g_devAnalogDoor`; the original toggle logic is untouched. Also the DEV "lights as hold-button" mode.

### Русский
- `DoorSystem::SetDoorAmount/GetDoorAmount` — непрерывная позиция двери 0..1,
  логический «closed» (AI) = `amount >= 0.5`. DEV-переключатель `g_devAnalogDoor`;
  исходная toggle-логика не тронута. Также DEV-режим «лампы как удержание».

---

## v2.20 — saves & achievements: two builds (system / local) / сохранения и достижения: две сборки (система / локально)

### English
- **Inverted `FNAF_LIVE_SAFE` semantics** (it controls both `Progress.cpp` and `Achievements.cpp`):
  - **default (no macro) = "regular"** — touches the Xbox system: save via XContent (`XShowDeviceSelectorUI` + `XContentCreateEx`), achievements via `XUserWriteAchievements`, Y opens the **system** list (`XShowAchievementsUI`).
  - **`FNAF_LIVE_SAFE` = "Live Safe"** — does NOT touch the system: saves/achievements to a local `save\fnaf_save.ini` / `save\fnaf_ach.ini` next to the .xex, no XUserWriteAchievements/XShowAchievementsUI/XContent.
- **Storage in `Progress.cpp`** moved behind `StorageOpen`/`StorageClose` helpers (the XContent branch and the local `save\` branch with `_mkdir("save")`); `Load`/`Save`/`LoadAchieve`/`SaveAchieve` are shared by both.
- **Achievements**: added `Achievements::ShowSystemUI()` (system build → `XShowAchievementsUI(0)`; Live Safe → false). In `main.cpp`, Y on the title: `if (gi.yToggle && !g_ach.ShowSystemUI()) g_achScreen = true;`
- The in-game toast and achievement screen remain as a fallback in both builds.

### Русский
- **Инвертирована семантика `FNAF_LIVE_SAFE`** (она управляет и `Progress.cpp`, и `Achievements.cpp`):
  - **по умолчанию (без макроса) = «обычная»** — трогает систему Xbox: сохранение через XContent (`XShowDeviceSelectorUI` + `XContentCreateEx`), достижения через `XUserWriteAchievements`, Y открывает **системный** список (`XShowAchievementsUI`).
  - **`FNAF_LIVE_SAFE` = «Live Safe»** — систему НЕ трогает: сохранение и достижения в локальный файл `save\fnaf_save.ini` / `save\fnaf_ach.ini` рядом с .xex, без XUserWriteAchievements/XShowAchievementsUI/XContent.
- **Хранилище в `Progress.cpp`** вынесено за хелперы `StorageOpen`/`StorageClose` (XContent-ветка и локальная `save\`-ветка с `_mkdir("save")`); `Load`/`Save`/`LoadAchieve`/`SaveAchieve` общие для обеих веток.
- **Достижения**: добавлен `Achievements::ShowSystemUI()` (системная сборка → `XShowAchievementsUI(0)`; Live Safe → false). В `main.cpp` Y на титуле: `if (gi.yToggle && !g_ach.ShowSystemUI()) g_achScreen = true;`
- Внутриигровые тост и экран достижений оставлены как fallback в обеих сборках.

---

## v2.19 — dB mixer + jumpscare fixes / dB-микшер + фиксы скримеров

### English

**Audio: dB layer (Clickteam/DirectSound → XAudio2)**
- `AudioSystem` now stores channel volume in **dB**, not linear 0..1 amplitude (`m_channelVolumeDb[32]`, `SetChannelVolume(dB)`). `PlayOnChannel` plays at the channel's current level; `Play(...)` stays a linear-amplitude one-shot.
- Helpers in `AudioSystem.h`: `DbToAmplitude(dB)` (10^(dB/20)), `CFVolumeToDb(0..100)` (20·log10(v/100)), `AmplitudeToDb(...)`. Uses C89 `pow/log10`, no C99 `powf/log10f` (VS2010).
- All `SetChannelVolume` calls moved to dB via `CFVolumeToDb(orig 0..100)` — values can be changed 1:1 against the original events.

**Random / dynamic events**
- **Freddy's "got in" laugh** now **random** from #56/#57/#58 (random(1,3)) + `running fast3`, volume by distance (bathrooms 20/35, kitchen 30/40, E-hall 40/60, corner 60/75, right door 80/100) — groups 390-405.
- **Pirate song2** (ch13): every 80 s 1/30 while Foxy is in the cove; 15 while watching CAM 1C, 5 otherwise (groups 269/274/275).
- **Breaths** (ch14, #22-25): every 100 s 1/3 when Bonnie/Chica are at the door and the camera is up (groups 276-283).
- **Circus** (ch15, #29): every 100 s 1/30 (group 270).
- **Music box** (ch22, #30): when Freddy enters the kitchen (groups 399/400).
- **Phone call without the 2.5 s delay** — immediately on entering the office (groups 361-365), nights 6/7 silent.

**Jumpscare fixes**
- Removed the fake screen shake from `RenderJumpscare`/`RenderScareFlash` (the original scares are static; motion comes from the animation itself).
- Removed the IT'S ME overlay from the kill animations (it was baked into `RenderJumpscare` for everyone); IT'S ME now only through its own rare hallucination (`RenderItsmeFlash` / the DEV "IT'S ME" item).
- **Starts on frame 0**: `scareElapsed` increments AFTER the first frame, so Chica (60 FPS) no longer skips frame 0.

### Русский

**Аудио: dB-прослойка (Clickteam/DirectSound → XAudio2)**
- `AudioSystem` теперь хранит громкость каналов в **dB**, а не в линейной амплитуде 0..1 (`m_channelVolumeDb[32]`, `SetChannelVolume(dB)`). `PlayOnChannel` проигрывает на текущем уровне канала, `Play(...)` остаётся линейной амплитудой для one-shot «сока».
- Помошники в `AudioSystem.h`: `DbToAmplitude(dB)` (10^(dB/20)), `CFVolumeToDb(0..100)` (20·log10(v/100)) и `AmplitudeToDb(...)`. Время компиляции — C89 `pow/log10`, без C99 `powf/log10f` (VS2010).
- Все вызовы `SetChannelVolume` переведены в dB через `CFVolumeToDb(ориг. 0..100)` — значения можно менять 1-в-1 по оригинальным event'ам.

**Случайные/динамические эвенты**
- **Смех Фредди «got in»** теперь **случайный** из #56/#57/#58 (random(1,3)) + `running fast3`, с громкостью по дистанции (в. туалеты 20/35, кухня 30/40, E-холл 40/60, угол 60/75, правая дверь 80/100) — groups 390-405.
- **Pirate song2** (ch13): каждые 80 с 1/30, пока Фокси в вороньем; 15 при взгляде на CAM 1C, 5 иначе (groups 269/274/275).
- **Breaths** (ch14, #22-25): каждые 100 с 1/3, когда Бонни/Чика у двери и включена камера (groups 276-283).
- **Circus** (ch15, #29): каждые 100 с 1/30 (group 270).
- **Music box** (ch22, #30): при заходе Фредди в кухню (groups 399/400).
- **Телефонный звонок без задержки 2.5 с** — сразу на входе в офис (groups 361-365), nights 6/7 — тишина.

**Фиксы скримеров**
- **Убран фейковый шейк экрана** из `RenderJumpscare` и `RenderScareFlash` — в оригинале скримеры статичны (движение даёт сама анимация).
- **Убран IT'S ME оверлей** из kill-анимаций (он был вшит в `RenderJumpscare` для всех); IT'S ME теперь только через свою редкую галлюцинацию (`RenderItsmeFlash` / пункт DEV «IT'S ME»).
- **Старт с нулевого кадра**: `scareElapsed` инкрементится ПОСЛЕ первого кадра, чтобы Chica (60 FPS) не пропускала frame 0.

---

## v2.18 — random events (IT'S ME + Golden Freddy) / случайные эвенты (IT'S ME + Golden Freddy)

### English
- **Footsteps by distance** (`OnAnimatronicMove`, group 198-244): Bonnie/Chica — `DEEP_STEPS` at 0.15 (far) / 0.30 (halls) / 0.40 (corners and doors), matching the original 10/30/40.
- **Freddy's laugh fixed**: movement now plays the `_1d/_2d/_8d` (#56/57/58) family in rotation, not `Laugh_Giggle_Girl_1` (#38 = Golden Freddy).
- **IT'S ME hallucination** — a rare flash: every ~20 s a 1/1000 roll (group 419); on success `WHISPERING` + a full-screen 4-frame cycle (`RenderItsmeFlash`, 12 Hz, Active 21 anim 0). Previously DEV-only.
- **Golden Freddy poster** (group 41/42/348): on monitor drop `random for pic` = random(1,100); when CAM 2B is empty and the roll < 2 — the "LET'S PARTY!" poster shows Golden Freddy's face (anim 75, handle 571). `GameRender::m_goldenRoll` (constructor -1, rolled in `RenderOffice`, read in `RenderCamera`). Bonnie/Chica in the frame take priority.

### Русский
- **Шаги по дистанции** (`OnAnimatronicMove`, group 198-244): Bonnie/Chica — `DEEP_STEPS` с громкостью 0.15 (далеко) / 0.30 (залы) / 0.40 (углы и двери), матчинг оригинальной градации 10/30/40.
- **Смех Фредди исправлен**: на перемещение играет семейство `_1d/_2d/_8d` (#56/57/58) по кругу, а не `Laugh_Giggle_Girl_1` (#38 = Golden Freddy).
- **IT'S ME галлюцинация** — редкий flash: каждый ~20 с бросок 1/1000 (group 419), при успехе `WHISPERING` + полноэкранный цикл 4 кадров (`RenderItsmeFlash`, 12 Гц, активн. 21 анимация 0). Раньше только из DEV.
- **Постер Golden Freddy** (group 41/42/348): при опускании планшета крутится `random for pic` = random(1,100); когда CAM 2B пуст и бросок < 2 — на постере «LET'S PARTY!» показывается лицо Golden Freddy (anim 75, handle 571). `GameRender::m_goldenRoll` (в конструкторе -1, крутится в `RenderOffice`, читается в `RenderCamera`). Bonnie/Chica в кадре имеют приоритет.

---

## v2.17 — DEV/debug menu / DEV/debug меню

### English
- **`Start + B`** opens the DEV menu from any state (B/Y closes):
  - **God mode** (A) — no power drain, no attacks (`Game::SetDebugGodMode`).
  - **Jump to night N** — instant start of any night 1..7.
  - **Force 6 AM** (`Game::DebugForceNightComplete`).
  - **Force power out** (`Game::DebugTriggerPowerOut`).
  - **Trigger jumpscare** — Freddy/Bonnie/Chica/Foxy + **Golden Freddy** (the distinct `Laugh_Giggle_Girl_1` giggle + the full-screen 571 flash).
  - **Sound test** — 22 sounds (`DEV_SOUNDS` in main.cpp).
  - **Unlock / Reset achievements** (`Achievements::UnlockAll/ClearAll`) + **Console ON/OFF** (the on-screen log; messages are always duplicated to the VS output via `printf`).
  - List of all 17 frames (storyboard).
- The DEV menu is also a **hidden title-menu entry** (press Up on the title → hidden position, A opens; `MENU_OPT_DEV` with no text/arrow).
- **Freddy "endo-head" flicker** (img_442) added to the title (`IMG_MENU_FLICK3`).
- New methods: `Game::SetDebugGodMode/DebugForceNightComplete/DebugTriggerPowerOut/DebugTriggerJumpscare`, `Achievements::UnlockAll/ClearAll`, `GameRender::RenderDevMenu`.

### Русский
- **`Start + B`** открывает DEV-меню из любого состояния (B/Y закрывает):
  - **God mode** (A) — без слива энергии и без атак (`Game::SetDebugGodMode`).
  - **Jump to night N** — мгновенный старт любой ночи 1..7.
  - **Force 6 AM** — завершение текущей ночи (`Game::DebugForceNightComplete`).
  - **Force power out** (`Game::DebugTriggerPowerOut`).
  - **Trigger jumpscare** — Freddy/Bonnie/Chica/Foxy + **Golden Freddy** (отдельный звук-«хихиканье» `Laugh_Giggle_Girl_1` + полноэкранная вспышка 571).
  - **Sound test** — 22 звука на прослушку (массив `DEV_SOUNDS` в main.cpp).
  - **Unlock / Reset achievements** (`Achievements::UnlockAll/ClearAll`) + **Console ON/OFF** (тогл on-screen лога; сообщения всегда дублируются в выход VS через `printf`).
  - Список всех 17 кадров (сториборд-раскадровка).
- DEV-меню также доступно как **скрытый пункт титульного меню** (нажать Up на титуле → скрытая позиция, A открывает; `MENU_OPT_DEV` без текста/стрелки).
- **Фликер Фредди «эндо-голова»** (img_442) добавлен на титул (`IMG_MENU_FLICK3`).
- Новые методы: `Game::SetDebugGodMode/DebugForceNightComplete/DebugTriggerPowerOut/DebugTriggerJumpscare`, `Achievements::UnlockAll/ClearAll`, `GameRender::RenderDevMenu`.

---

## v2.16 — audio mixer (1:1) / аудио микшер (1:1)

### English
- **Channel volume** in `AudioSystem`: `PlayOnChannel` / `SetChannelVolume` (re-applied to a live voice), 32 channels — matching the original `Speaker` channels.
- **Office on channels** (`frame_3` group 15): `BuzzFan` (ch1), `ColdPresc B` (ch2), `BallastHum` (ch3), plus the proximity loops `robotvoice` (ch21, muted) and `EerieAmbience` (ch18, muted).
- **`TickAudioMixer`** every frame in the office: fan 25/10 by camera, hum muted by camera/light, phone 100/50/0 (viewing/mute), proximity ambience (robotvoice/EerieAmbience rise as animatronics approach the doors/office).

### Русский
- **Канальная громкость** в `AudioSystem`: `PlayOnChannel` / `SetChannelVolume` (re-применяется на живой голос), 32 канала — матчинг оригинальных `Speaker`-каналов.
- **Офис на каналах** (`frame_3` group 15): `BuzzFan` (ch1), `ColdPresc B` (ch2), `BallastHum` (ch3), плюс прoxимити-лупы `robotvoice` (ch21, muted) и `EerieAmbience` (ch18, muted).
- **`TickAudioMixer`** каждый кадр в офисе: вентилятор 25/10 по камере, гумбас mут при камере/свете, телефон 100/50/0 (viewing/mute), прoxимити-амбиент (robotvoice/EerieAmbience растут при приближении аниматроников к дверям/офису).

---

## v2.15 — transitions/fades + sound / переходы/фейды + звук

### English

**Transitions and effects**
- **Frame fades** — a `ScreenFade` transition machine in `main.cpp`: black overlay with alpha, 1:1 durations from `docs/FRAME_TRANSITIONS.md` (disclaimer 1010/1010, "what day" 1010 out, "next day" 1010 in/900 out, gameover 1010 in, newspaper 2000/2000; menu/office/jumpscare — hard cut). `GameRender::DrawFade(alpha)`, logic freezes during fade-out.
- **Disclaimer like the original** — auto-advance after ~40 s, skip with any button (A/Start/B) after a short lock, no "PRESS START", 1010 ms fade-in/out.

**Save**
- **Device selector** (`XShowDeviceSelectorUI`) moved to the correct async pattern (XOVERLAPPED + `XHasOverlappedIoCompleted` + `XGetOverlappedResult`); fixed the `NULL`-overlapped bug that made the dialog not appear. Shows once at load (after the `.pak` check).

**Sound (1:1)**
- **Newspaper no longer hangs** — a separate `adCounter` (the menu used to zero the shared `menuFrameCounter` every frame).
- **Ambience starts AFTER the fade**, not during the blackout (edge detectors `g_officeAmb` / `g_menuAmb` / `g_nightBlip`).
- **`blip3` on the night-number card** ("what day", group 2) — 1:1.
- **Phone only in the office** — `TickPhoneCall` gates on `GAME_STATE_PLAYING` (the call used to leak onto the newspaper).
- **`robotvoice`** — removed the wrong loud loop; per the dump it starts muted (vol 0) and only rises via proximity triggers (not ported).
- **Full sound map** in `docs/AUDIO.md` (106 events, 53 samples, `Speaker` act #1/#11/#12/#17 semantics, channel/volume map).

### Русский

**Переходы и эффекты**
- **Frame fades** — машина переходов `ScreenFade` в `main.cpp`: чёрный оверлей с альфой, длительности 1:1 из `docs/FRAME_TRANSITIONS.md` (дисклеймер 1010/1010, «what day» 1010 out, «next day» 1010 in/900 out, gameover 1010 in, газета 2000/2000; меню/офис/джампскейр — жёсткий cut). `GameRender::DrawFade(alpha)`, freeze логики на fade-out.
- **Дисклеймер как оригинал** — авто-переход через ~40 с, пропуск любой кнопкой (A/Start/B) после короткого lock, без «PRESS START», fade-in/out 1010 мс.

**Сохранение**
- **Окно выбора накопителя** (`XShowDeviceSelectorUI`) переведено на корректный асинхронный паттерн (XOVERLAPPED + `XHasOverlappedIoCompleted` + `XGetOverlappedResult`); убран баг с `NULL`-оверлаппедом, из-за которого окно не появлялось. Появляется один раз при загрузке (после проверки `.pak`).

**Звук (1:1)**
- **Газета больше не «зависает»** — отдельный счётчик `adCounter` (меню раньше обнуляло общий `menuFrameCounter` каждый кадр).
- **Эмбиент стартует ПОСЛЕ fade**, а не во время затемнения (edge-детекторы `g_officeAmb` / `g_menuAmb` / `g_nightBlip`).
- **`blip3` на карточке номера ночи** («what day», group 2) — 1:1.
- **Телефон только в офисе** — `TickPhoneCall` гейтуется на `GAME_STATE_PLAYING` (раньше звонок влезал на газету).
- **`robotvoice`** — убран ошибочный громкий луп; по дампу он на старте офиса muted (vol 0) и поднимается только прoxимити-триггерами (не портирован).
- **Полная карта звуков** в `docs/AUDIO.md` (106 событий, 53 семпла, семантика `Speaker` act #1/#11/#12/#17, карта каналов и громкостей).

---

## v2.14 — achievements / достижения

### English
- **In-game achievements** (`Achievements.h/.cpp`): 10 achievements from `achievements.xml`, unlock via triggers, progress in `fnaf_save:\fnaf_ach.ini`, title screen (Y) + toast.
- **Two builds** via `#define FNAF_LIVE_SAFE` in `Achievements.cpp` (inverted in v2.20): default — system (`XUserWriteAchievements`, works on RGH/JTAG), with the macro — purely local (`save\` next to the .xex).
- **Menu sound restored** (the lost `g_audio.Play` on the disclaimer→menu transition).

### Русский
- **Внутриигровые достижения** (`Achievements.h/.cpp`): 10 ачивок из `achievements.xml`, разблокировка по триггерам, прогресс в `fnaf_save:\fnaf_ach.ini`, экран на титуле (Y) + toast.
- **Две сборки** через `#define FNAF_LIVE_SAFE` в `Achievements.cpp` (v2.20 инвертирована): по умолчанию — система (`XUserWriteAchievements`, работает и на RGH/JTAG), с макросом — чисто локальная (`save\` рядом с .xex).
- **Звук в меню восстановлен** (потерянные `g_audio.Play` на переходе дисклеймер → меню).

---

## v2.8 — Perspective (clean-room shader) / Perspective (clean-room шейдер)

### English
- **Ported the HWA shader** `RPanorama.fx` (parabola) instead of the wrong sine: render target + `SpriteBatch::BeginSceneCapture/EndSceneCapture/DrawPerspective`; ps_3_0 in `SpriteBatch.cpp`.
- **`Resolve()`** moved to the real 10-argument XDK signature.
- **Disabled auto depth-stencil** (not needed, frees EDRAM for the capture RT).
- The perspective tuner reworked for the shader (ZOOM/CENTER_Y/CURVE).

### Русский
- **Портирован HWA-шейдер** `RPanorama.fx` (парабола) вместо ошибочной синусоиды: рендер-таргет + `SpriteBatch::BeginSceneCapture/EndSceneCapture/DrawPerspective`; ps_3_0 в `SpriteBatch.cpp`.
- **`Resolve()`** переведён на реальную 10-аргментную сигнатуру XDK.
- **Отключён auto depth-stencil** (не нужен, освобождает EDRAM под capture RT).
- Тюнер перспективы переделан под шейдер (ZOOM/CENTER_Y/CURVE).

---

## Earlier (base) / Ранее (база)

### English
- Xbox-only XContent save (`[freddy]` INI, `fnaf_save.ini`), Delete-key wipe, DEMO = dead code.
- Full port of the frames/sprites/HUD: office, cameras, power-out, jumpscares, perspective, tablet, night flow — from the CTFAK dumps (`docs/*.md`).

### Русский
- Xbox-only XContent-сохранение (`[freddy]` INI, `fnaf_save.ini`), Delete-key wipe, DEMO = dead code.
- Полный порт кадров/спрайтов/HUD офиса, камер, павер-аута, джампскейров, перспективы, таблета, ночного флоу — из CTFAK-дампов (`docs/*.md`).

---

## Remaining for full 1:1 (fine-grained audio tuning) / Осталось для полного 1:1 (точечная доводка звука)

### English
The mixer is done (dB layer + channels); only the rarest/niche items remain:

1. ~~Footstep distance (`deep steps` 10..40 by room, mute when overlapping).~~
2. ~~Freddy's kitchen music (ch22 music box) and pirate song (ch13).~~
   ~~Oven (ch10)~~ — now ported (plays when Chica is in the kitchen, v2.22).
3. ~~Golden Freddy giggle #38 + jumpscare~~ — full auto-activation done in v2.22 (poster → giggle on CAM 2B → office appearance → creepy kill).
4. ~~Phone call without delay~~ — removed (v2.19).
5. ~~Camcorder/tape-eject (ch6) on monitor-up (group 144)~~ — wired in v2.22.
6. **Ch16/ch24 proximity grading** — done by room on entry (v2.19); the "walk into the office" (`yellow bear`/Freddy inside) remains an approximation, as does the exact `vol zone` overlap for ch18/ch21.

We do not generate the SPA or sign the title (external tool/signing).

### Русский
Микшер готов (dB-прослойка + каналы); остались самые редкие/нишевые вещи:

1. ~~Дистанция шагов (`deep steps` 10..40 по комнатам, мут вплотную).~~
2. ~~Музыка Фредди в кухне (ch22 music box) и pirate song (ch13).~~
   ~~Oven (ch10)~~ — портировано (играет, когда Chica на кухне; v2.22).
3. ~~Golden Freddy хиханье #38 + джампскейр~~ — полная авто-активация сделана в v2.22 (постер → гига на CAM 2B → появление в офисе → crash-скример).
4. ~~Звонок без задержки~~ — убрано (v2.19).
5. ~~Camcorder/tape-eject (ch6) при подъёме планшета (group 144)~~ — привязано в v2.22.
6. **Прокси-градация** ch16/ch24 — сделана по комнатам при заходе (v2.19); «сунуть в офис» (`yellow bear`/Freddy inside) и точная зона `vol zone` для ch18/ch21 остаются аппроксимацией.

Не генерируем SPA и не подписываем титл (внешний инструмент/подпись).