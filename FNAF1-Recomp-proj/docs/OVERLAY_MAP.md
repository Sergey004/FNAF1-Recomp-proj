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
| Смерть "died" | obj 2 "static" (ink 9/0) + obj 35 "blip flash" (НЕПРОЗРАЧНЫЙ белый/шум, ink 1/0) поверх ЧЁРНОГО фона — ГРОМКАЯ шумовая буря, фона-картинки нет | у нас не рендерится (сейчас сразу gameover) — задел на будущее |
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

- Экран "died" (шумовая буря между скримером и game over): чёрный фон +
  непрозрачный blip-цикл + статик.
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
