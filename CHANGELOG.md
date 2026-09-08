# Changelog — FNAF1 Recomp (Xbox 360)

Заметки «что у нас уже есть» и «что осталось». Версии соответствуют тегам в
комментариях кода (`v2.8`, `v2.14`, `v2.15`, `v2.16`) и историческим заметкам.

## v2.20 — сохранения и достижения: две сборки (система / локально)

- **Инвертирована семантика `FNAF_LIVE_SAFE`** (она управляет и
  `Progress.cpp`, и `Achievements.cpp`):
  - **по умолчанию (без макроса) = «обычная»** — трогает систему Xbox:
    сохранение через XContent (`XShowDeviceSelectorUI` + `XContentCreateEx`),
    достижения через `XUserWriteAchievements`, Y открывает **системный** список
    (`XShowAchievementsUI`).
  - **`FNAF_LIVE_SAFE` = «Live Safe»** — систему НЕ трогает: сохранение и
    достижения в локальный файл `save\fnaf_save.ini` / `save\fnaf_ach.ini`
    рядом с .xex, без XUserWriteAchievements/XShowAchievementsUI/XContent.
- **Хранилище в `Progress.cpp`** вынесено за хелперы `StorageOpen`/`StorageClose`
  (XContent-ветка и локальная `save\`-ветка с `_mkdir("save")`); `Load`/`Save`/
  `LoadAchieve`/`SaveAchieve` общие для обеих веток.
- **Достижения**: добавлен `Achievements::ShowSystemUI()` (системная сборка →
  `XShowAchievementsUI(0)`; Live Safe → false). В `main.cpp` Y на титуле:
  `if (gi.yToggle && !g_ach.ShowSystemUI()) g_achScreen = true;`
- Внутриигровые тост и экран достижений оставлены как fallback в обеих сборках.

## v2.19 — dB-микшер + фиксы скримеров

### Аудио: dB-прослойка (Clickteam/DirectSound → XAudio2)
- `AudioSystem` теперь хранит громкость каналов в **dB**, а не в линейной
  амплитуде 0..1 (`m_channelVolumeDb[32]`, `SetChannelVolume(dB)`).
  `PlayOnChannel` проигрывает на текущем уровне канала, `Play(...)` остаётся
  линейной амплитудой для one-shot «сока».
- Помошники в `AudioSystem.h`: `DbToAmplitude(dB)` (10^(dB/20)),
  `CFVolumeToDb(0..100)` (20·log10(v/100)) и `AmplitudeToDb(...)`.
  Время компиляции — C89 `pow/log10`, без C99 `powf/log10f` (VS2010).
- Все вызовы `SetChannelVolume` переведены в dB через `CFVolumeToDb(ориг. 0..100)`
  — значения можно менять 1-в-1 по оригинальным event'ам.

### Случайные/динамические эвенты
- **Смех Фредди «got in»** теперь **случайный** из #56/#57/#58 (random(1,3)) +
  `running fast3`, с громкостью по дистанции (в. туалеты 20/35, кухня 30/40,
  E-холл 40/60, угол 60/75, правая дверь 80/100) — groups 390-405.
- **Pirate song2** (ch13): каждые 80 с 1/30, пока Фокси в вороньем; 15 при
  взгляде на CAM 1C, 5 иначе (groups 269/274/275).
- **Breaths** (ch14, #22-25): каждые 100 с 1/3, когда Бонни/Чика у двери и
  включена камера (groups 276-283).
- **Circus** (ch15, #29): каждые 100 с 1/30 (group 270).
- **Music box** (ch22, #30): при заходе Фредди в кухню (groups 399/400).
- **Телефонный звонок без задержки 2.5 с** — сразу на входе в офис (groups
  361-365), nights 6/7 — тишина.

### Фиксы скримеров
- **Убран фейковый шейк экрана** из `RenderJumpscare` и `RenderScareFlash` — в
  оригинале скримеры статичны (движение даёт сама анимация).
- **Убран IT'S ME оверлей** из kill-анимаций (он был вшит в `RenderJumpscare`
  для всех); IT'S ME теперь только через свою редкую галлюцинацию
  (`RenderItsmeFlash` / пункт DEV «IT'S ME»).
- **Старт с нулевого кадра**: `scareElapsed` инкрементится ПОСЛЕ первого кадра,
  чтобы Chica (60 FPS) не пропускала frame 0.

## v2.18 — случайные эвенты (IT'S ME + Golden Freddy)

- **Шаги по дистанции** (`OnAnimatronicMove`, group 198-244): Bonnie/Chica —
  `DEEP_STEPS` с громкостью 0.15 (далеко) / 0.30 (залы) / 0.40 (углы и двери),
  матчинг оригинальной градации 10/30/40.
- **Смех Фредди исправлен**: на перемещение играет семейство `_1d/_2d/_8d`
  (#56/57/58) по кругу, а не `Laugh_Giggle_Girl_1` (#38 = Golden Freddy).
- **IT'S ME галлюцинация** — редкий flash: каждый ~20 с бросок 1/1000
  (group 419), при успехе `WHISPERING` + полноэкранный цикл 4 кадров
  (`RenderItsmeFlash`, 12 Гц, активн. 21 анимация 0). Раньше только из DEV.
- **Постер Golden Freddy** (group 41/42/348): при опускании планшета крутится
  `random for pic` = random(1,100); когда CAM 2B пуст и бросок < 2 — на постере
  «LET'S PARTY!» показывается лицо Golden Freddy (anim 75, handle 571).
  `GameRender::m_goldenRoll` (в конструкторе -1, крутится в `RenderOffice`,
  читается в `RenderCamera`). Bonnie/Chica в кадре имеют приоритет.

## v2.17 — DEV/debug меню

- **`Start + B`** открывает DEV-меню из любого состояния (B/Y закрывает):
  - **God mode** (A) — без слива энергии и без атак (`Game::SetDebugGodMode`).
  - **Jump to night N** — мгновенный старт любой ночи 1..7.
  - **Force 6 AM** — завершение текущей ночи (`Game::DebugForceNightComplete`).
  - **Force power out** (`Game::DebugTriggerPowerOut`).
  - **Trigger jumpscare** — Freddy/Bonnie/Chica/Foxy + **Golden Freddy**
    (отдельный звук-«хихиканье» `Laugh_Giggle_Girl_1` + полноэкранная вспышка 571).
  - **Sound test** — 22 звука на прослушку (массив `DEV_SOUNDS` в main.cpp).
  - **Unlock / Reset achievements** (`Achievements::UnlockAll/ClearAll`) + **Console ON/OFF**
    (тогл on-screen лога; сообщения всегда дублируются в выход VS через `printf`).
  - Список всех 17 кадров (стoриборд-раскадровка).
- DEV-меню также доступно как **скрытый пункт титульного меню** (нажать Up на
  титуле → скрытая позиция, A открывает; `MENU_OPT_DEV` без текста/стрелки).
- **Фликер Фредди «эндо-голова»** (img_442) добавлен на титул (`IMG_MENU_FLICK3`).
- Новые методы: `Game::SetDebugGodMode/DebugForceNightComplete/
  DebugTriggerPowerOut/DebugTriggerJumpscare`, `Achievements::UnlockAll/ClearAll`,
  `GameRender::RenderDevMenu`.

## v2.16 — аудио микшер (1:1)

- **Канальная громкость** в `AudioSystem`: `PlayOnChannel` / `SetChannelVolume`
  (re-применяется на живой голос), 32 канала — матчинг оригинальных
  `Speaker`-каналов.
- **Офис на каналах** (`frame_3` group 15): `BuzzFan` (ch1), `ColdPresc B` (ch2),
  `BallastHum` (ch3), плюс прoxимити-лупы `robotvoice` (ch21, muted) и
  `EerieAmbience` (ch18, muted).
- **`TickAudioMixer`** каждый кадр в офисе: вентилятор 25/10 по камере, гумбас
  mут при камере/свете, телефон 100/50/0 (viewing/mute), прoxимити-амбиент
  (robotvoice/EerieAmbience растут при приближении аниматроников к дверям/офису).

## v2.15 — переходы/фейды + звук (текущая итерация)

### Переходы и эффекты
- **Frame fades** — машина переходов `ScreenFade` в `main.cpp`: чёрный оверлей с
  альфой, длительности 1:1 из `docs/FRAME_TRANSITIONS.md`
  (дисклеймер 1010/1010, «what day» 1010 out, «next day» 1010 in/900 out,
  gameover 1010 in, газета 2000/2000; меню/офис/джампскейр — жёсткий cut).
  `GameRender::DrawFade(alpha)`, freeze логики на fade-out.
- **Дисклеймер как оригинал** — авто-переход через ~40 с, пропуск любой кнопкой
  (A/Start/B) после короткого lock, без «PRESS START», fade-in/out 1010 мс.

### Сохранение
- **Окно выбора накопителя** (`XShowDeviceSelectorUI`) переведено на корректный
  асинхронный паттерн (XOVERLAPPED + `XHasOverlappedIoCompleted` +
  `XGetOverlappedResult`); убран баг с `NULL`-оверлаппедом, из-за которого окно
  не появлялось. Появляется один раз при загрузке (после проверки `.pak`).

### Звук (1:1)
- **Газета больше не «зависает»** — отдельный счётчик `adCounter` (меню раньше
  обнуляло общий `menuFrameCounter` каждый кадр).
- **Эмбиент стартует ПОСЛЕ fade**, а не во время затемнения (edge-детекторы
  `g_officeAmb` / `g_menuAmb` / `g_nightBlip`).
- **`blip3` на карточке номера ночи** («what day», group 2) — 1:1.
- **Телефон только в офисе** — `TickPhoneCall` гейтуется на `GAME_STATE_PLAYING`
  (раньше звонок влезал на газету).
- **`robotvoice`** — убран ошибочный громкий луп; по дампу он на старте офиса
  muted (vol 0) и поднимается только прoxимити-триггерами (не портирован).
- **Полная карта звуков** в `docs/AUDIO.md` (106 событий, 53 семпла, семантика
  `Speaker` act #1/#11/#12/#17, карта каналов и громкостей).

## v2.14 — достижения

- **Внутриигровые достижения** (`Achievements.h/.cpp`): 10 ачивок из
  `achievements.xml`, разблокировка по триггерам, прогресс в `fnaf_save:\fnaf_ach.ini`,
  экран на титуле (Y) + toast.
- **Две сборки** через `#define FNAF_LIVE_SAFE` в `Achievements.cpp` (v2.20
  инвертирована): по умолчанию — система (`XUserWriteAchievements`, работает и
  на RGH/JTAG), с макросом — чисто локальная (`save\` рядом с .xex).
- **Звук в меню восстановлен** (потерянные `g_audio.Play` на переходе
  дисклеймер → меню).

## v2.8 — Perspective (clean-room шейдер)

- **Портирован HWA-шейдер** `RPanorama.fx` (парабола) вместо ошибочной
  синусоиды: рендер-таргет + `SpriteBatch::BeginSceneCapture/EndSceneCapture/
  DrawPerspective`; ps_3_0 в `SpriteBatch.cpp`.
- **`Resolve()`** переведён на реальную 10-аргментную сигнатуру XDK.
- **Отключён auto depth-stencil** (не нужен, освобождает EDRAM под capture RT).
- Тюнер перспективы переделан под шейдер (ZOOM/CENTER_Y/CURVE).

## Ранее (база)

- Xbox-only XContent-сохранение (`[freddy]` INI, `fnaf_save.ini`), Delete-key wipe,
  DEMO = dead code.
- Полный порт кадров/спрайтов/HUD офиса, камер, павер-аута, джампскейров,
  перспективы, таблета, ночного флоу — из CTFAK-дампов (`docs/*.md`).

---

## Осталось для полного 1:1 (точечная доводка звука)

Микшер готов (dB-прослойка + каналы); остались самые редкие/нишевые вещи:

1. ~~**Дистанция шагов** (`deep steps` 10..40 по комнатам, мут вплотную).~~
2. ~~**Музыка Фредди в кухне** (ch22 music box) и **pirate song** (ch13).~~
   — **oven (ch10)** пока не портировано (играет, когда Chica на кухне).
3. **Golden Freddy хиханье #38 + джампскейр** (frame 14 «creepy start»,
   активация `yellow bear` → giggle на взгляд CAM 2B → отсчёт → crash-скример).
   Постер (1/100) и DEV-скример есть, полная авто-активация — нет.
4. ~~**Звонок без задержки**~~ — убрано (v2.19).
5. **Camcorder/tape-eject (ch6)** — при подъёме планшета (group 144); пока
   whir/static на switch, лента-выброс не привязан.
6. **Прокси-градация** ch16/ch24 — сделана по комнатам при заходе (v2.19);
   «сунуть в офис» (`yellow bear`/Freddy inside) остаётся аппроксимацией.

Не генерируем SPA и не подписываем титл (внешний инструмент/подпись).