# FNAF1-Recomp — Five Nights at Freddy's для Xbox 360

Реимплементация FNAF1 на «сыром» Xbox 360 XDK (D3D9 + XAudio2, VS2010/PPC,
C++03 — без лямбд и nullptr) с прицелом на 1:1 соответствие оригинальной PC-версии.
Проект вырос в **фундамент для всей классической серии**: FNAF 2/3/4, Sister
Location и Ultimate Custom Night садятся на тот же каркас как модули.

> **Правило проекта:** источник истины — **только дамп игры**, снятый нашим
> собственным чистroom-инструментом **CTFAK-CPP**. Вики, чужие порты и
> «я так помню» — не источники, а лишь чек-листы того, что искать в дампе.
> Любое осознанное отклонение от дампа помечается в коде и CHANGELOG как
> «deliberate deviation».

## Статус (v2.29)

- **FNAF1 — играбельна 1:1**: ночи 1–7 + Custom Night, полный ИИ аниматроников
  по дампу (группы событий), скримеры, отключка света, Golden Freddy
  (постер 1/100 → офис → kill-экран + force-close), сохранения/ачивки в
  XContent, импорт лузального сейва, звуки/анимации выверены по дампу
  (`fps = speed × 0.6`).
- **Фундамент серии (v2.28–2.29)**: контракт `AppModule` (ядро ↔ игра),
  реестр модулей FNAF1/2/3, стриминговый загрузчик пака (под SL),
  генерация GPU-паков и per-game таблиц ассетов для всех игр.
- Дамбы сняты и лежат рядом с инструментом: FNAF1/2/3/4 + SL
  (27/26/19/36 фреймов событий, паки 241 КБ…1.5 ГБ).

Подробная история изменений: [CHANGELOG.ru.md](CHANGELOG.ru.md) /
[CHANGELOG.en.md](CHANGELOG.en.md).

## Архитектура

```
CORE (app shell, main.cpp + общие системы)
  D3D9-цикл 60 Гц · SpriteBatch/рендер-таргеты · TextRenderer
  PakLoader (eager + streaming) · AudioSystem (XAudio2 dB-микшер)
  InputSystem (XInput) · фейды · XContent-сейвы · ачивки · DebugConsole
────────────────────────────────────────────────────────────
AppModule (контракт: Name/PakName/Load/Tick/Render/WantsExit)
────────────────────────────────────────────────────────────
FNaF1Module (активна)   FNaF2Module   FNaF3Module   … (FNAF4/SL/UCN)
```

- Ядро не включает игровые заголовки; модуль не владеет устройством —
  получает `AppServices{audio, pak, batch, text}`.
- Реестр: `include/AppRegistry.h`. Полный план миграции по стадиям 2–7:
  [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Сборка и запуск

1. **Среда**: XDK-сборка — VS2010 + Xbox 360 XDK (`FNAF1-Recomp-proj.vcxproj`).
   Кросплатформенность не цель; PC-путь в коде существует лишь как заглушки.
2. **Новые файлы** добавлять в `.vcxproj` вручную (файлы проекта синхронизируются
   между машинами; в репо их список = `src/` + `include/`).
3. **Ассеты**: рядом с XEX должен лежать `fnaf1.pak` (см. «Конвейер ассетов»).
   Без него — системное окно «Missing fnaf1.pak».
4. **Запуск**: Xenia для быстрой итерации или консоль (RGH/JTAG, игра с HDD).
   Аргумент командной строки `1..7` — стартовая ночь (dev).
5. **Две сборки**: по умолчанию — системная (сейв/ачивки в XContent);
   `FNAF_LIVE_SAFE` — лайвсейф-вариант (сейв в `game:\save\freddy`,
   ачивки локально).

## Управление (консольная схема)

| Действие | Кнопка |
|---|---|
| Осмотр офиса | Левый стик ←→ |
| Свет у двери | LB / RB |
| Двери | LT / RT (аналогово) |
| Планшет камер | A |
| Навигация по камерам | D-Pad |
| Пауза | Start |
| Назад | B |
| Нос Фредди в офисе | Y |
| DEV-меню | Start+B |
| Браузер спрайтов | LB+RB (в меню) |
| Perspective-тюнер | L3+R3 (Y — сброс ручки) |

## Конвейер ассетов (CTFAK-CPP)

Инструмент: `/home/user/FNAF1-Recomp/ctfak-cppnew/ctfak-cpp` (clean-room C++,
Readme внутри). Полный цикл для любой игры серии:

```bash
# 1) дамп (картинки/звуки/packed data):
./build/ctfak-cpp -path <game>.exe -tool "Dump Everything"        -closeonfinish
# 2) события (Groups/ON/DO по фреймам) — источник логики:
./build/ctfak-cpp -path <game>.exe -tool "Events Listing"          -closeonfinish
# 3) структура (application.json: объекты, фреймы, анимации):
./build/ctfak-cpp -path <game>.exe -tool "Export Structure as JSON" -closeonfinish
# 4) GPU-ready пак + таблица ассетов (DXT-текстуры + PCM, big-endian 'FNAF'):
./build/ctfak-cpp -path <game>.exe -tool "Recomp Pack"             -closeonfinish
```

Результат в `build/Dumps/<Game>/`:

- `Events/*.txt` + `ALL_EVENTS.txt` — события (условия/действия/параметры);
- `JSON/application.json` — объекты/фреймы/анимации;
- `Images/`, `Sounds/`, `Packed Data/` — сырые ассеты;
- `RecompPack/<game>.pak` + `pak_manifest.json` + `asset_mapping_<game>.hpp`.

**Таблицы ассетов** (`include/assets/asset_mapping_*.hpp`): каждая в своём
namespace (`fnaf1::assets`, `fnaf2::assets`, …, `sisterlocation::assets`) с
`ASSET_COUNT`; игровой модуль включает только свою — коллизий нет. Это
ground-truth {index, filename, w, h, alpha} для сверки и для per-game таблиц
ассетов на stage 4.

**Стриминг**: `PakLoader::LoadStreaming()` («скольжение по файлу»: резидентны
только таблицы, блобы читаются по `dataOff` на первом использовании,
`PreloadAsync` тянет следующую комнату в фоне) — активирует SL-модуль; пак
SL ~1.5 ГБ, жадная загрузка туда не влезает (512 МБ UMA). FNAF1/2/3/4 живут
на жадном `Load()`. Игры идут **с HDD**, поэтому DVD-стек XDK (XFileCache,
ReadFileScatter, physical sort keys) не применим.

## Сохранения

- Файл — оригинальный **`freddy`** без расширения (INI: `[freddy]`,
  `level`/`beatgame`/`beat6`/`beat7`). Системная сборка — XContent-контент
  `freddy`; Live Safe — `game:\save\freddy`.
- При старте системная сборка находит лузальный `freddy` рядом с XEX и
  предлагает импорт в XContent («Do you want to import the save found in the
  game folder?» → «Import complete. Please restart the game.» + выход).
- Ачивки — отдельно (`fnaf_ach.ini`), дефолт через `XUserWriteAchievements`.

## Документация (`docs/`)

| Файл | О чём |
|---|---|
| `ARCHITECTURE.md` | ядро+модули, дампы всех игр, стадии миграции |
| `AI_MECHANICS.md` | ИИ аниматроников 1:1 по группам дампа |
| `CAMERA_FINDINGS.md` | таблица камер-фидов (каноничная) |
| `AUDIO.md` | звуковой банк, каналы, действиями-актами |
| `OVERLAY_MAP.md` | оверлеи/статики всех экранов |
| `PERSPECTIVE.md`, `PERSPECTIVE_TUNER.md` | парабола офиса и тюнер |
| `TABLET_FLIP.md` | анимация планшета |
| `RESTORE.md`, `XDK_NOTES.md`, `pak_format.md` | восстановление, заметки XDK, формат пака |

## Версионирование

`v2.XX` проставляется в баннерах `src/main.cpp` и в обоих чейнджлогах
одновременно. Правило при спорах о поведении: смотреть группы событий в
`Events/*.txt`, а не вики; отклонение от дампа = отдельная запись
«deliberate deviation».

## Дисклеймер

Фанатская реимплементация для собственной консоли. Требуется легально
купленная оригинальная игра — ассеты извлекаются из неё же. Не
распространять ассеты и собранные образы.
