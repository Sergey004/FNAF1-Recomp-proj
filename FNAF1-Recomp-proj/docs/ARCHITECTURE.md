# АРХИТЕКТУРА (v2.28): ядро + игровые модули (FNAF 1/2/3)

## Правило источника

Для каждой игры «оригинал» — ТОЛЬКО её дамп. Дампы сняты нашим CTFAK-CPP
(clean-room, `/home/user/FNAF1-Recomp/ctfak-cppnew/ctfak-cpp`):

| Игра | exe | Фреймов | Групп | Картинок | Звуков | Объектов | Fusion build |
|---|---|---|---|---|---|---|---|
| FNAF1 | FiveNightsatFreddys.exe (Steam v1.132) | 17 | 435 (офис) | 605 | ~50 | — | 288 |
| FNAF2 | FiveNightsatFreddys2.exe | 27 | **1301** (офис 751) | 804 | 66 (wav) | 452 | 288 |
| FNAF3 | FiveNightsatFreddys3.exe | 26 | **1526** (офис 773) | 1066 | 70 (wav) | 577 | 288 |
| FNAF4 | FiveNightsatFreddys4.exe | 19 | **1159** (level 480) | 1281 | 72 (wav) | 405 | 288 |
| SL | SisterLocation.exe (950 МБ!) | 36 | **3037** (custom level 501) | **5260** | 392 (362 wav + 30 mp3) | 788 | **286** |

Дампы: `ctfak-cpp/build/Dumps/Five Nights at Freddys{, 2, 3, 4}/Sister Location/`
(Events/*.txt + ALL_EVENTS.txt, JSON/application.json, Images/, Sounds/, Packed Data/, RecompPack/).

Особенности дампов:
- **FNAF4**: офис = фрейм «level» 1300×768; миниигры Plushtrap/BB отдельными
  фреймами; Cutscenes 5120×3840; nightmare jumpscare; lockbox/extras.
- **SL**: это сплошные покомнатные фреймы (Elevator/Vent crawl/Ballora
  Gallery/Breaker Room/Funtime Auditorium/Parts and Service ×2/Under Desk…),
  «8-bit Baby Game» шириной **12800×720**, «Final Encounter» 251 группа,
  Custom menu/level; Fusion build **286** (старее остальных — инструмент
  переварил); в паке 178 PCM + **214 passthrough** (mp3-звуки) — PakLoader
  на Xbox потребует декод/транскод на этапе репака (заметка на stage SL).

## Слои

```
┌────────────────────────────────────────────────────────────┐
│ CORE (app shell) — main.cpp + общие системы                │
│  D3D9-устройство и цикл 60 Гц, SpriteBatch/рендер-таргеты, │
│  TextRenderer, PakLoader, AudioSystem (XAudio2 dB-микшер), │
│  InputSystem (XInput), фейды, XShowMessageBoxUI-обвязка,   │
│  XContent/Progress-обвязка, ачивки-пайплайн, DebugConsole  │
├────────────────────────────────────────────────────────────┤
│ AppModule (контракт, include/AppModule.h)                  │
│  Name/PakName/Load/Unload/Tick/Render/WantsExit            │
├──────────────┬──────────────┬──────────────────────────────┤
│ FNaF1Module  │ FNaF2Module  │ FNaF3Module                  │
│ (живёт пока  │ (заглушка)   │ (заглушка)                   │
│ в main.cpp)  │              │                              │
└──────────────┴──────────────┴──────────────────────────────┘
```

Ядро НЕ включает игровые заголовки и говорит с игрой только через
AppModule. Модуль не владеет устройством и циклом: получает AppServices
(audio/pak/batch/text) и очередит квадры между Begin/End ядра.

Реестр: `AppRegistry` (include/AppRegistry.h, src/AppRegistry.cpp).
Сейчас активен всегда FNAF1; селектор на титуле — stage 5.

## Контракт (include/AppModule.h)

```cpp
struct AppServices { AudioSystem* audio; PakLoader* pak;
                     SpriteBatch* batch; TextRenderer* text; };
class AppModule {
  virtual const char* Name()   const = 0;  // "FNAF1"
  virtual const char* PakName() const = 0; // "fnaf1.pak"
  virtual bool Load(AppServices&) = 0;     // false -> missing-pak UI ядра
  virtual void Unload() = 0;
  virtual void Tick(f32 dt) = 0;           // фикс 60 Гц
  virtual void Render() = 0;               // очередит квадры, не трогает device
  virtual bool WantsExit() const = 0;      // force-close Голден Фредди и т.п.
};
```

## Что общего у трёх игр (остаётся в core)

- Формат паков — то же семейство Clickteam 2.5 (build 288 у всех трёх);
  PakLoader переиспользуется, меняется только имя .pak.
- XAudio2 dB-микшер с каналами — схема 1:1 как в FNAF1 (у каждой игры
  своя раскладка каналов, но АПИ общий).
- XInput, фейды, message boxes, XContent-обвязка, ачивки-пайплайн,
  SpriteBatch/TextRenderer, DebugConsole, DEV-браузер спрайтов.

## Что у каждой игры своё (модуль)

| | FNAF1 | FNAF2 | FNAF3 |
|---|---|---|---|
| Экран офиса | 1600×720, панорама+Perspective.mfx | 1600×768, панорама, БЕЗ фонарика нет | 2000×768, панорама шире |
| Ключевая механика | камеры+двери+свет | фонарик, вентиляция, шкатулка, маска | фонарик+шкатулка, вентиляция, PHANTOM'ы, minigames |
| Кадры-особенности | creepy start/end | dream 2500×768, 8bit (108 групп), error/error 2, rare1 ×3 | cutscenes (190 групп), атари-кадры 3072×2304: BB/Mangle/Toy Chica/GFreddy/RWQFSFASXC/Marion, bad/good end |
| Сейв | `freddy` (XContent) | свой layout (сверять с дампом) | свой layout |
| Звук | ~50 сэмплов, ogg/wav | 66 wav | 70 wav |

Внимание: размеры экранов в дампах 1024×768/1600×768 — это координаты
Fusion, НЕ финальные 1280×720; правило пересчёта то же, что у FNAF1.

## План миграции (не ломая работающую FNAF1)

- **Stage 1 (v2.28, сделано)**: контракт AppModule, реестр, модули;
  FNAF1 продолжает работать напрямую из main.cpp; баннер печатает модуль.
- **Stage 2**: состояние игры (GAME_STATE_* диспетчер и тик-блок)
  переезжает в FNaF1Module::Tick/Render; main.cpp оставляет только ядро
  (device/loop/input/fades). `exit(0)` Голден Фредди → `RequestExit()`.
- **Stage 3**: аудио-колбэки (OnJumpscare/OnPowerOut/...) → внутрь модуля;
  core даёт только AudioSystem*.
- **Stage 4**: PakAssets.h делится на общий минимум и per-game таблицы;
  g_pak грузит пак выбранного модуля; Progress получает per-game layout
  (общая XContent-обвязка, разные имена файлов/поля).
- **Stage 5**: boot-селектор (LB/RB на титуле ядра до Load модуля) +
  missing-pak флоу по PakName() модуля.
- **Stage 6**: FNAF2 — по дампу `Five Nights at Freddys 2` (офис = фрейм 3,
  751 группа: счётчики "viewing"/"lit?", объекты "new chica" и пр.).
- **Stage 7**: FNAF3 — по дампу `Five Nights at Freddys 3` (office = фрейм 3
  2000×768; cutscenes; атари-минигеймы — отдельные фреймы 3072×2304).

## Верификация CTFAK-CPP (тест 12.09.2026)

Оба exe прочитались без ошибок: стандартное CF2.5-шифрование снялось,
zlib/LZ4 распаковались, PNG/звуки/event-листинги полные. Замечаний к
инструменту нет; события рендерятся читаемо (условия/действия/параметры).

## Repack: Recomp Pack (тул #10 CTFAK-CPP)

Восстановлен 12.09.2026: код жил в `src/plugins/RecompPack.{h,cpp}` с
первых коммитов, но не был ни зарегистрирован в `builtinTools()`
(src/plugins/Plugin.cpp), ни добавлен в CMakeLists — поэтому в списке
тулов его не было. Теперь 10 tool(s), имя пака выводится из имени игры
(fnaf1/fnaf2/fnaf3.pak).

Собранные паки (GPU-ready: DXT1/DXT5 текстуры + PCM-звуки, big-endian,
магия 'FNAF', формат PakLoader):

| Пак | Текстуры | Звуки | Размер |
|---|---|---|---|
| RecompPack/fnaf2.pak | 804 (202 DXT1 + 602 DXT5) | 66 PCM | 241 МБ |
| RecompPack/fnaf3.pak | 1066 (237 DXT1 + 829 DXT5) | 70 PCM | 218 МБ |

Рядом: `pak_manifest.json` (полная раскладка) и `asset_mapping.hpp`
(ground-truth таблица {handle,name,w,h,alpha} — подfeed для per-game
таблиц stage 4).
