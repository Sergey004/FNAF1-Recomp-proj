# Переходы кадров (fade) — выгрузка из дампов

v2.76. Источник: `Dumps/<игра>/JSON/application.json` → `frames[].fadeIn` / `frames[].fadeOut`
(чанки 13113/13115 = fade-in, 13114/13116 = fade-out; см. `ctfak-cpp/src/format/Frame.cpp`).
Полный листинг событий с заголовками «Fade-in / Fade-out» — в
`Dumps/<игра>/Events/frame_N_<имя>_events.txt`.

## Правило (одинаково во всех пяти играх)

* Переход только один: `module=STDT`, `name=FADE`, `moduleFile=cctrans.dll`, цвет `[0,0,0,0]`
  (чёрный). Ни RECT-, ни MASK-переходов нет нигде.
* Длительность — **сырое поле `duration` в миллисекундах**: 560 / 900 / 1010 / 1130 / 2000 / 2030.
  (Аннотация `(~Ns)` в листингах events — артефакт 50 Гц, как и таймеры: см. TIMERS_CFT.md.)
* Направление задаётся **чанком**, а не флагом: 13113/13115 = fade-in, 13114/13116 = fade-out.
  Поле `flags` у Transition — отдельный безымянный битфилд: во всех играх у fade-in он 0,
  у fade-out — 1, **кроме** F4 (кадры 6, 7, 17, 18: у fade-in тоже 1). Семантика флага
  в дампе неизвестна; направление берём по чанку, длительность применяем как есть.
* Семантика перехода в CFT: при «Jump to frame» сначала играет **fade-out старого** кадра,
  потом грузится новый кадр и играет его **fade-in**. Оба — чёрные, оба поверх всех слоёв.
  Значит переход в порте **отложенный**: старый экран живёт и гаснет, новый появляется из чёрного.
* Оверлей fade обязан рисоваться **последним** в кадре и **вне**
  `BeginSceneCapture/EndSceneCapture` — внутри capture его выгнет парабола перспективы.

## Таблица по кадрам (fade-in / fade-out, мс)

| кадр | F1 (17) | | F2 (27) | | F3 (26) | | F4 (19) | | SL (36) | |
|---|---|---|---|---|---|---|---|---|---|---|
| 0  | Frame 17 | 1010/1010 | Frame 17 | —/1010 | Frame 17 | —/1010 | Frame 17 | 1010/1010 | — | — |
| 1  | title | —/— | title | —/— | title | —/— | titlescreen | 1130/1010 | нет | нет |
| 2  | what day | —/1010 | what day | —/1010 | what day | —/1010 | what night | 1130/— | frame-level | нет |
| 3  | Frame 1 (офис) | —/— | Frame 1 | —/— | Frame 1 | —/— | level | —/— | fade'ов | нет |
| 4  | died | —/— | static | —/— | static | —/— | game over | —/— | вообще | нет |
| 5  | freddy | —/— | next day | 1010/900 | next day | 1010/900 | night win | 900/2000 | | |
| 6  | next day | 1010/900 | gameover | —/— | gameover | —/— | intro to plushtrap | 1010/1010 | | |
| 7  | wait | —/— | wait | —/— | wait | —/— | plushtrap game | 1010/— | | |
| 8  | gameover | 1010/— | ad | 2000/2000 | ad | 2000/2000 | game over 2 | —/— | | |
| 9  | the end | 2000/2000 | the end | 2000/2000 | bad end | 2000/2000 | lockbox | 2000/2000 | | |
| 10 | ad (газетка) | 2000/2000 | the end 2 | 2000/2000 | good end | 2000/2000 | extras | —/— | | |
| 11 | the end 2 | 2000/2000 | the end 3 | 2000/2000 | the end 2 | 2000/2000 | load extras | —/— | | |
| 12 | customize | 560/— | customize | —/— | rare1 | —/— | Cutscenes | —/2030 | | |
| 13 | the end 3 | 2000/2000 | dream | —/— | rare2 | —/— | ending | 2000/2000 | | |
| 14 | creepy start | —/— | error | —/— | rare3 | —/— | test | 1010/1010 | | |
| 15 | creepy end | —/— | error 2 | —/— | demo end | 1010/1010 | nightmare jumpscare | —/— | | |
| 16 | end of demo | 1130/1010 | rare1 | —/— | cutscenes | —/— | demo | 2000/2000 | | |
| 17 | — | | rare1 | —/— | end | —/— | intro to plushtrap | 1010/1010 | | |
| 18 | | | rare1 | —/— | load | —/— | BB game | 1010/— | | |
| 26 | | | demo end | 1010/1010 | | | | | | |

Остальные кадры F2 (19-25), F3 (19-25) и F4 — без переходов (жёсткий смен).

**SL: frame-level переходов нет НИ ОДНОГО.** SL гасит экраны объектом `"fade out"`
(длительность в дампе не выгружается — unknown in dump) и отдельным кадром
`21 "red fade out"`, который в порте уже нарисован картинкой (img 1437).

## Соответствие «наш экран → кадр дампа» (то, что реально реализовано в порту)

### F1 (src/main.cpp: FadeOutMs/FadeInMs + ScreenFade)
| наш state | кадр | fade-out | fade-in |
|---|---|---|---|
| DISCLAIMER | 0 Frame 17 | 1010 | 1010 |
| MENU (title) | 1 title | — | — |
| NIGHT_START (карточка) | 2 what day | 1010 | — |
| INTRO_AD (газетка) | 10 ad | 2000 | 2000 |
| CUSTOMIZE | 12 customize | — | 560 |
| NIGHT_COMPLETE (6 AM) | 6 next day | 900 | 1010 |
| GAME_OVER | 8 gameover | — | 1010 |
| OFFICE / POWER_OUT / JUMPSCARE | 3 / 5 / 14 | — | — |

Кадры `the end` / `the end 2` / `the end 3` (2000/2000) и `end of demo` (1130/1010)
в порте **не выразимы**: отдельных состояний для них нет (концовки свёрнуты в
NIGHT_COMPLETE, демо-сборка — мёртвый код).

### F2 (src/FNaF2Game.cpp: FadeOutMs/FadeInMs + RequestScreen)
| наш экран | кадр | fade-out | fade-in |
|---|---|---|---|
| DISCLAIMER | 0 Frame 17 | 1010 | — |
| TITLE | 1 title | — | — |
| AD (газетка) | 8 ad | 2000 | 2000 |
| NIGHTSTART (карточка) | 2 what day | 1010 | — |
| OFFICE | 3 Frame 1 | — | — |
| STATIC | 4 static | — | — |
| NEXTDAY (6 AM) | 5 next day | 900 | 1010 |
| DREAM | 13 dream | — | — |
| ERROR / ERROR2 | 14 / 15 | — | — |
| END5 / END6 / END7 | 9 / 10 / 11 | 2000 | 2000 |
| остальные | — | — | — |

### F3 (src/FNaF3Game.cpp)
| наш экран | кадр | fade-out | fade-in |
|---|---|---|---|
| DISCLAIMER | 0 Frame 17 | 1010 | — |
| TITLE | 1 title | — | — |
| AD | 8 ad | 2000 | 2000 |
| NIGHTSTART | 2 what day | 1010 | — |
| OFFICE | 3 Frame 1 | — | — |
| STATIC6 | 4 static | — | — |
| NEXTDAY | 5 next day | 900 | 1010 |
| ENDBAD / ENDGOOD / END2 | 9 / 10 / 11 | 2000 | 2000 |
| остальные | — | — | — |

### F4 (src/FNaF4Game.cpp)
| наш экран | кадр | fade-out | fade-in |
|---|---|---|---|
| DISCLAIMER | 0 Frame 17 | 1010 | 1010 |
| TITLE | 1 titlescreen | 1010 | 1130 |
| NIGHTSTART (карточка) | 2 what night | — | 1130 |
| BEDROOM | 3 level | — | — |
| NIGHTWIN | 5 night win | 2000 | 900 |
| GAMEOVER / GAMEOVER2 | 4 / 8 | — | — |
| INTRO | 6 intro to plushtrap | 1010 | 1010 |
| PLUSH | 7 plushtrap game | — | 1010 |
| LOCKBOX | 9 lockbox | 2000 | 2000 |
| LOADX | 11 load extras | — | — |
| EXTRAS | 10 extras | — | — |
| CUTSCENE | 12 Cutscenes | 2030 | — |
| ENDING | 13 ending | 2000 | 2000 |
| TEST | 14 test | 1010 | 1010 |
| NJSCARE | 15 | — | — |
| BB | 18 BB game | — | 1010 |

### SL
Реализаций нет: в дампе SL нет frame-level переходов. Роутер переходов между комнатами
(`SCR_LOAD`, «go to»-роутер) и объектный `"fade out"` оставлены как есть.

## Механика в порту

* `include/ScreenFade.h` — общий контроллер (phase 0/1/2, alpha, timer, dur, pending).
  F1 (`src/main.cpp`), F2, F3, F4 используют один и тот же.
* `SpriteBatch::DrawSolid(x,y,w,h,argb)` — текстуро-независимый сплошной квад
  (1x1 белая текстура + спрайт-PS: `tex2D(white) * colour = colour`). Цвет `0xAARRGGBB`,
  альфа в **старшем** байте, бленд SRCALPHA/INVSRCALPHA (не premultiplied) —
  чёрный с альфой a = `(a<<24)`.
  До v2.76 fade F1 брал белый кадр `img_23` **из пака F1** — в модульном режиме там
  лежит чужой кадр, поэтому fade теперь не зависит от пака.
* Отложенный переход: `RequestScreen(target, pendingNight)` — если у текущего экрана
  есть fade-out, переключение откладывается до конца гашения; иначе — жёсткий смен
  плюс fade-in нового экрана. `pendingNight` стартует ночь в момент переключения,
  чтобы dwell карточки ночи начинался вместе с ней, а не во время гашения.
* Во время fade-out старая логика **продолжает тикать** (в CFT старый кадр жив до
  конца перехода). F1 — исключение: там логика замораживается (`main.cpp`:
  `if (g_fade.phase != 1)`) — так было проверено юзером, не меняем.

## Проверка

Харнес: `tools/offline/offline fnaf2 <Images> out` → `f2_fade_0..4.ppm` (филмстрип
заставки на альфах 0/25/50/75/100 %).
