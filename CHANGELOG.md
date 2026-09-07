# Changelog — FNAF1 Recomp (Xbox 360)

Заметки «что у нас уже есть» и «что осталось». Версии соответствуют тегам в
комментариях кода (`v2.8`, `v2.14`, `v2.15`, `v2.16`) и историческим заметкам.

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
- **Две сборки** через `#define FNAF_LIVE_SAFE` в `Achievements.cpp`:
  c `XUserWriteAchievements` (работает и на RGH/JTAG) / чисто локальная.
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

Микшер готов; осталась мелкая доводка триггеров/градаций:

1. **Дистанция шагов** (`deep steps` 10..40 по комнатам, мут вплотную).
2. **Кухня/музыка Фредди** (ch10 oven / ch22 music box по присутствию и взгляду
   на CAM 6), **pirate song** (ch13: 15 при взгляде на к.ворон, 5 иначе).
3. **Смех Фредди** по «freddy got in» (#56/57/58) и Golden Freddy (#38) — сейчас
   рекомп играет hourly giggle.
4. **Звонок без задержки 2.5 с** (в оригинале — сразу на входе в офис).
5. **Прокси-градация** ch16/ch24 для Фредди при заходе — пока аппроксимирована
   одним «watched» в `TickAudioMixer`.

Не генерируем SPA и не подписываем титл (внешний инструмент/подпись).