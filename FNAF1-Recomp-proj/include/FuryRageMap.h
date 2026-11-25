/**
 * FuryRageMap.h - GENERATED data, do not edit by hand.
 * Source: ctfak dump of furys-rage.exe - level frame (5920x5400, 50 px
 * tiles). Instances keep the dump's draw order (painter's order per
 * layer). Kind: 0 sprite, 1 quick backdrop (tiled ground),
 * 2 backdrop (stretched rect). Collision: solid=1 blocks the fighter's
 * hitbox (FuryRageGame.cpp); alpha is the object's own blend.
 * VS2010/C++03.
 */
#ifndef FNAF_FURYRAGE_MAP_H
#define FNAF_FURYRAGE_MAP_H

namespace fnaf {

static const int kFuryMapW = 118;
static const int kFuryMapH = 108;
static const int kFuryTile = 50;

struct FuryMapInst { unsigned short img; short left; short top;
                  unsigned short w, h;
                  unsigned char layer; unsigned char solid;
                  unsigned char kind; unsigned char alpha; };
                                             // 0 sprite, 1 quick
                                             // backdrop (tiled),
                                             // 2 backdrop (rect)
static const FuryMapInst kFuryMapInsts[] = {
    {   442,      0,   1075,  3840,  1080, 0, 0, 1, 255 }, // Quick Backdrop 6
    {   444,      5,   2160,  3840,  1080, 0, 0, 1, 255 }, // Quick Backdrop 4
    {   448,      0,   3240,  7680,  1080, 0, 0, 1, 255 }, // Quick Backdrop 9
    {    60,      0,      0,  3840,  1080, 0, 0, 0, 255 }, // Active 3
    {   774,      0,   4325,  3840,  1080, 0, 0, 0, 255 }, // Active 12
    {    62,      0,      0,  7680,  1080, 1, 0, 1, 255 }, // Quick Backdrop
    {   438,      0,   1080,  7680,  1080, 1, 0, 1, 255 }, // Quick Backdrop 3
    {   445,      0,   2160,  7680,  1080, 1, 0, 1, 255 }, // Quick Backdrop 7
    {   447,      0,   3240,  7680,  1080, 1, 0, 1, 255 }, // Quick Backdrop 5
    {   777,      0,   4320,  3840,  1080, 1, 0, 0, 255 }, // Active 13
    {   729,   1085,    500,   281,   204, 2, 1, 0, 255 }, // crate
    {    31,    713,   -128,   191,    51, 2, 0, 0, 105 }, // Active 4
    {    10,    340,    430,   400,   400, 2, 0, 0, 255 }, // Active
    {   823,    -10,    695,  1908,   300, 2, 0, 0, 105 }, // floor
    {   729,   2325,    500,   281,   204, 2, 1, 0, 255 }, // crate
    {   729,   2905,    505,   281,   204, 2, 1, 0, 255 }, // crate
    {   729,    870,    495,   281,   204, 2, 1, 0, 255 }, // crate
    {  1029,    712,     58,   850,   750, 2, 0, 0, 255 }, // giant animdude
    {   729,   3495,    505,   281,   204, 2, 1, 0, 255 }, // crate
    {    64,      0,      0,  7680,  1080, 3, 0, 1, 255 }, // Quick Backdrop 2
    {   443,      0,   1080,  7680,  1080, 3, 0, 1, 255 }, // Quick Backdrop 10
    {   446,     10,   2160,  7680,  1080, 3, 0, 1, 255 }, // Quick Backdrop 8
    {   449,      0,   3240,  7680,  1080, 3, 0, 1, 255 }, // Quick Backdrop 11
    {   776,      0,   4320,  3840,  1080, 3, 0, 2, 255 }, // Backdrop
    {   776,  -1470,   4320,  3840,  1080, 3, 0, 2, 255 }, // Backdrop
    {   362,     49,    113,    50,    50, 3, 0, 0, 255 }, // Lives 5
    {   462,     53,    168,   102,    18, 3, 0, 0, 255 }, // Active 7
    {   205,  -2475,   -255,    17,    62, 3, 0, 0, 255 }, // Lives 4
    {   828,     50,     34,    32,    62, 3, 0, 0, 255 }, // Lives 2
    {   128,    -54,     48,    20,    39, 3, 0, 0, 255 }, // test1
    {   128,    -54,     87,    20,    39, 3, 0, 0, 255 }, // test2
    {   128,    -54,    126,    20,    39, 3, 0, 0, 255 }, // test3
    {   273,   1891,   1050,    37,    86, 3, 0, 0, 255 }, // score
    {   358,   1882,   1064,    10,    19, 3, 0, 0, 255 }, // score 2
    {   827,     50,     34,    32,    62, 3, 0, 0, 255 }, // Lives
    {   826,     44,    107,    50,    50, 3, 0, 0, 255 }, // Lives 3
    {   128,    565,    -71,    20,    39, 4, 0, 0, 255 }, // I test
    {   876,     40,    891,   138,   138, 4, 0, 0, 255 }, // have animdude
    {   184,     50,   1029,   146,    28, 4, 0, 0, 255 }, // Active 14
    {   850,    188,   1021,    18,    35, 4, 0, 0, 255 }, // animdude
    {   846,   1392,   -149,   142,    19, 4, 0, 0, 255 }, // Active 16
    {   847,   1400,   -115,   131,    23, 4, 0, 0, 255 }, // Active 17
    {   848,   1423,    -78,    74,    23, 4, 0, 0, 255 }, // Active 18
    {   866,   1421,    -42,   145,    18, 4, 0, 0, 255 }, // Active 19
    {   830,   1467,    400,   346,   256, 4, 0, 0, 255 }, // Active 20
};

static const int kFuryMapInstCount = (int)(sizeof(kFuryMapInsts)/sizeof(kFuryMapInsts[0]));

} // namespace fnaf

#endif // FNAF_FURYRAGE_MAP_H
