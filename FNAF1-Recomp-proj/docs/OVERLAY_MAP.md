# КАРТА ВСЕХ ОВЕРЛЕЙ ОРИГИНАЛА (v2.7.5)
# Полный проход по FiveNights-55.mfa: раскладки фреймов + события + пиксели
# (ctfak-cpp: FrameLayout + EventsListing + Image Dumper + application.json)

## МЕХАНИКА ПЕРЕКЛЮЧЕНИЯ (события Frame 1)

    группа 81: viewing == 0 (монитор ОПУЩЕН) -> HIDE obj 46 (белая вспышка),
               HIDE obj 42 (СТАТИК), HIDE obj 43 (REC), HIDE obj 50 (рамка)
    группа 82: viewing > 0  (монитор ПОДНЯТ) -> SHOW obj 42, 43, 50

    Статик-объект Frame 1: obj 42 "Active", anim [18,20,12,13,14,15,16,17]
    speed 100, ink=1 (semi-transparent) coeff=100 -> альфа 155/255 (~61%),
    VisibleAtStart = FALSE. Никогда не создаётся событиями — только SHOW/HIDE.

## СЛОИ ПО ЭКРАНАМ (как в оригинале)

| Экран (фрейм) | Оверлеи | Наши действия в v2.7.5 |
|---|---|---|
| Disclaimer "Frame 17" | НЕТ оверлеев (только img_605 + Text за экраном) | уже так |
| Title "title" | obj 4 фон img_431 (ink 1/0) -> obj 2 "static" (ink 9/0, ВСЕГДА виден, anim [18,20,12..17] speed 99) -> obj 12 "blip flash 2" (430-439, редкие тёмные вспышки, 95% прозрачный img_430) -> twitch img_440/441/442 в направлениях 12/13/14 obj 4 | уже так (титул одобрен) |
| Что за день "what day" | obj 35 "blip flash": anim [23,23,23,4,25,6,8,9,10,21,22] speed 75 — ТРИ СПЛОШНЫХ БЕЛЫХ кадра, потом шум (кадры 4/25/6.. с ВРОЖДЁННОЙ альфой ~50%) | уже так (BLIP_FRAMES) |
| ОФИС "Frame 1" | **НИЧЕГО анимированного.** Зерно ЗАПЕЧЕНО в img_39 (1600x720, средняя яркость ~10/255) | **УБРАН DrawStaticOverlay** (был наш баг с v2.6) |
| Камеры (viewing>0) | obj 42 статик 61% (ПОД картой/метками), obj 43 "Active 2" = МИГАЮЩИЙ КРАСНЫЙ REC: anim [img_7 (красный 50x50), img_5 (полностью прозрачный)] speed 2 @(68,52), obj 50 "frame" img_11 = 99.2% прозрачный (0.8% белых пикселей — мелкие белые элементы поверх), obj 46 белая вспышка по blip==1 | статик оставлен; **ДОБАВЛЕН красный REC**; рамка уже рисуется |
| Смерть "died" | obj 2 "static" (ink 9/0) + obj 35 "blip flash" (НЕПРОЗРАЧНЫЙ белый/шум, ink 1/0) поверх ЧЁРНОГО фона — ГРОМКАЯ шумовая буря, фона-картинки нет | **v2.62: рендерится** — статик-цикл + один проход блимпа ([23,23,23,4,25,6,8,9,10,21,22] @ 45 fps, потом объект уничтожается) + луп «static» (ch1, 100); фаза держит 10 с (таймер 10000 мс), потом gameover |
| Power out "freddy" | obj 152 "Active" anim [326,307,348,308..325] speed 60 backTo 5 + obj 2 static (ink 9/0) + obj 35 blip flash (создаётся группой G3) | у нас своя реализация фликера без статики — задел на будущее |
| 6 AM "next day" | НЕТ оверлеев: только 5/AM/6 цифры + счётчики | **УБРАН static 0.12** |
| Game over "gameover" | НЕТ оверлеев: backdrop img_358 + img_471 + Text/counter за экраном | **УБРАН static 0.55** |
| Custom night "customize" | НЕТ fullscreen-оверлеев (только кнопки) | уже так |

## ПИКСЕЛЬНАЯ СВОДКА (Image Dumper, 605 картинок)

    img_12/18/20 (статик)  1280x720  A=255 у 100% пикселей, средний цвет ~34-42/255
                           -> ТЁМНО-серый плотный шум, непрозрачные пиксели;
                              прозрачность даёт ТОЛЬКО ink-эффект (61%)
    img_4/25 (шум blip)    ~50% пикселей A=0, ~50% A=255 -> врождённая 50% альфа
    img_23 (blip база)     сплошной белый 1280x720
    img_11 ("frame")       99.2% A=0; белый только на мелких элементах
    img_7 / img_5 (REC)    красный 50x50 (166,0,0) / полностью прозрачный 69x69
    img_39 (офис)          1600x720, средняя яркость 10.5/255 — зерно внутри
    img_608 ("lights")     1600x253, 90.8% A=0 — жёлтый свет-градиент (хайлай
                           коридоров при включении света; у нас пока не рисуется)

## ЗАКРЫТЫЕ ВОПРОСЫ

1. "Помехи на геймплее" — подтверждено: это был НАШ баг (DrawStaticOverlay в
   RenderOffice). В оригинале офис чистый, статик живёт только на планшете.
2. img_11 — НЕ виньетка и НЕ пустышка: редкие белые элементы UI планшета.
3. REC-огонёк камеры — восстановлен по obj 43 (кадры 7/5, скорость 2).
4. ink=9 у титульного "static" (obj 2) — другой id эффекта (не 1); вид титула
   не трогаем, он одобрен.

## ЧТО МОЖНО ДОБАВИТЬ ПОТОМ (не в v2.7.5)

- ~~Экран "died" (шумовая буря между скримером и game over)~~ — **сделано в v2.62**
  (RenderDiedBurst + луп «static» + фаза 10 с; см. таблицу выше).
- Свет в офисе: obj 147 "lights" img_608 @(0,-78), VisibleAtStart=false,
  SHOW@G431 / HIDE@G411-433 — рисуется при зажатом свете.
- Белая вспышка камеры: obj 46 (img_23) создаётся при blip==1 (G16).

------------------------------------------------------------------------------
v2.7.6 ADDENDUM -- WHY EVERYTHING ON LAYER 0 IS BENT (Perspective.mfx)
------------------------------------------------------------------------------
Frame 1 also carries Andos' Perspective.mfx extension object
(objInfo 40, layer 1, instance (-22,-22), size 1324x754, serialized as
PANORAMA / HORIZONTAL / Zoom=300). No event references it, so it runs
forever: it re-projects the flat layer 0 (office scene AND camera feeds)
through a per-column sin curve -- center column 754 px tall, visible
screen edges 469/470 px. Consequences for the layer map above:
  * every 'scene' entry (office bg/fan/doors/panels, feeds, power-out,
    jump scares) is drawn flat, then re-projected (DrawBentInstance);
  * outside the bent band (top/bottom wedges at the screen edges) the
    FLAT scene stays visible -- exactly like the original's grab+blit;
  * layer 2+ UI (static, REC, bezel, labels, map, HUD) is unaffected.
Full evidence chain: docs/PERSPECTIVE.md.

------------------------------------------------------------------------------
v2.45 ADDENDUM -- THE CAMERA STATIC'S ALPHA IS EVENT-DRIVEN (group 13)
------------------------------------------------------------------------------
The v2.7.5 note above ("статик 61%") is HALFWAY right: 0.61 is what the
serialized object data says (obj 42, ink 1, coeff 100), but it is NOT what
runs. Frame 1 event group 13 has NO trigger condition (ON System = every
frame) and two actions on obj 42:
    act #65  "set alpha coefficient" := Ext(42, value 0)   (its alterable[0])
    act #31  alterable[0] := 150 + Random(50) + (Ext(42, value 1) ? 15)
    groups 15/14: alterable[1] := Random(3) — on game start and every ~20 s
So the LIVE coefficient is ~150..229 -> alpha = 1 - coeff/255 ~= 0.10..0.41
(avg ~0.25): the light breathing grain of the real monitor. The serialized
coeff 100 applies only before the first event tick. Measured on a real
original screenshot (Pirate Cove, feed area): mean luma 11.9/255, p95 34 —
impossible at fixed 0.61 (would be ~24 even over a black feed), matches
~0.15..0.2 at the capture moment.
Cross-check that act #65 is the alpha-coefficient setter: the title's
"blip flash 2" (obj 12) is ~95% transparent in-game (OVERLAY_MAP's own
observation) although its data says ink 1/0 (opaque) — only a per-frame
event-set coefficient explains it; the title groups 1/4/5 use the same
act #65 on 'static'/'Active 2'/'blip flash 2'.
The dense white-out on cam switches is group 16's blip flash (obj 46,
FLASH_SEQ, opaque/blip frames) — a different object, unaffected.
Port: RenderCamera re-rolls the same formula every frame (v2.45). The title
static keeps the approved look (obj 2 is ink 9 — a different effect id).
v2.46 user decision: the CAMERA static is FIXED at CAM_STATIC_ALPHA = 0.48
(a touch more transparent than the title's 0.61) — the 0.10..0.41 group-13
flicker reads as almost no noise on the dark room art on HW. The event
truth above (group 13 formula) stays as the documented deviation label;
RenderCamera carries the note.

CAM MAP BUTTONS (v2.45, same session): the map outlines img_164/145 carry
NO button plates — the dark backing of each "CAM xA" button is a separate
60x40 plate instance: obj 75/76/77/81/82/85/86/89/91/94/95, ALL img_167
(gray, hotspot 29,19), one per camera; the selected cam's plate is blinked
green img_166 / gray img_167 on top of it. The white texts img 165-177
(31x25) sit OVER the plates. The port drew the bare texts only (no plates)
and hid the selected name under its blink plate — fixed: all plates, then
the blink, then the texts. The big location label ("Pirate Cove", img_73
family, instance (832,292)) is text-only in the original too — no plate
(measured: the pixels between its letters are at static level, p50 ~36).
