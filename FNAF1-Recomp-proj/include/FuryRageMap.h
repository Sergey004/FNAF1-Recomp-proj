/**
 * FuryRageMap.h - GENERATED data, do not edit by hand.
 * Source: ctfak dump of furys-rage.exe - level frame (5920x5400, 50 px
 * tiles). Instances keep the dump's draw order (painter's order per
 * layer). Kind: 0 sprite, 1 quick backdrop (tiled ground),
 * 2 backdrop (stretched rect). Collision: solid=1 blocks the fighter's
 * hitbox (FuryRageGame.cpp).
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
                  unsigned char kind; };  // 0 sprite, 1 quick
                                             // backdrop (tiled),
                                             // 2 backdrop (rect)
static const FuryMapInst kFuryMapInsts[] = {
    {    60,      0,      0,  3840,  1080, 0, 0, 1 }, // Quick Backdrop 6
    {   220,    -13,   -145,   347,   265, 0, 0, 1 }, // Quick Backdrop 4
    {   206,   -161,   -305,   400,   400, 0, 0, 1 }, // Quick Backdrop 9
    {   213,    -78,    -50,   157,   100, 0, 0, 0 }, // Active 3
    {   511,    -74,    -39,   148,    79, 0, 0, 0 }, // Active 5
    {   455,   -160,   -399,   320,   400, 0, 0, 0 }, // freddy hit
    {   526,   -144,   -184,   288,   368, 0, 1, 0 }, // Active 6
    {   665,      0,      0,  1920,  1080, 0, 0, 0 }, // snack
    {   808,    -15,    -15,    32,    32, 0, 0, 0 }, // Active 9
    {   807,    -15,    -15,    32,    32, 0, 0, 0 }, // bouncing ball
    {   786,   -119,   -120,   240,   240, 0, 0, 0 }, // block fx
    {   764,   -184,    -48,   232,   232, 0, 0, 0 }, // Active 10
    {   774,      0,   4325,  3840,  1080, 0, 0, 0 }, // make pow 1
    {   777,      0,   4320,  3840,  1080, 1, 0, 0 }, // little pow
    {     3,   -130,    450,    12,    18, 1, 0, 0 }, // pow
    {     3,   -205,    510,    12,    18, 1, 0, 0 }, // sparkle
    {    31,    713,   -128,   191,    51, 2, 0, 1 }, // Quick Backdrop
    {    10,    340,    430,   400,   400, 2, 0, 1 }, // Quick Backdrop 3
    {     5,    660,    490,    83,    83, 2, 0, 1 }, // Quick Backdrop 7
    {     1,    660,    573,    83,    83, 2, 0, 1 }, // Quick Backdrop 5
    {   729,   1085,    500,   281,   204, 2, 0, 0 }, // Active 12
    {     1,    115,    530,    83,    83, 2, 0, 0 }, // Active 13
    {     5,    -38,    -38,    83,    83, 2, 0, 0 }, // Perspective
    {     1,    -38,    -38,    83,    83, 2, 0, 0 }, // offset
    {     8,    470,    760,    88,    68, 2, 0, 0 }, // playsel
    {    21,   1850,    -10,    66,  1092, 2, 1, 0 }, // crate
    {    65,   1168,     -4,   746,  1092, 2, 0, 0 }, // Active 4
    {   823,    -10,    695,  1908,   300, 2, 1, 0 }, // Active
    {   192,   1354,     -4,   540,  1092, 2, 0, 0 }, // punch box
    {    21,      0,      0,    66,  1092, 2, 1, 0 }, // punch box 2
    {    36,     10,    844,  1942,    70, 2, 1, 0 }, // walk target 1
    {    36,     34,    775,  1942,    70, 2, 0, 0 }, // punch box
    {    36,     24,    629,  1942,    70, 2, 1, 0 }, // punch box 2
    {   210,     18,    678,  1942,    50, 2, 0, 0 }, // move box
    {   105,   -125,   -125,   250,   250, 2, 0, 0 }, // block right
    {     3,    318,   -196,    12,    18, 2, 0, 0 }, // scroll sensor 1
    {     3,    550,   1150,    12,    18, 2, 0, 0 }, // floor
    {    57,    945,    557,    32,    32, 2, 0, 0 }, // scroll sensor 2
    {     3,    632,   -148,    12,    18, 2, 0, 0 }, // block left
    {    63,      7,    699,    60,    60, 2, 0, 0 }, // block bottom
    {    63,   1925,    695,    60,    60, 2, 0, 0 }, // to front
    {    69,   1465,    349,   128,   770, 2, 0, 0 }, // block top
    {    69,   3890,    350,   128,   770, 2, 0, 0 }, // to back
    {    69,   3925,    350,   128,   770, 2, 0, 0 }, // Active 2
    {    69,   3990,    350,   128,   770, 2, 0, 0 }, // enemy attack speed
    {    69,   3970,    350,   128,   770, 2, 0, 0 }, // jump counter
    {    69,   2121,    375,   128,   770, 2, 0, 0 }, // screen follow
    {    69,   2697,    365,   128,   770, 2, 0, 0 }, // enemies?
    {    69,   3273,    357,   128,   770, 2, 0, 0 }, // make enemy
    {     3,    178,   -214,    12,    18, 2, 1, 0 }, // make enemy
    {   220,   1233,   -315,   347,   265, 2, 0, 0 }, // create trigger 1
    {     3,    480,   -126,    12,    18, 2, 0, 0 }, // boss trigger
    {     3,    400,   -120,    12,    18, 2, 0, 0 }, // boss trigger 2
    {   207,    110,   -540,   400,   400, 2, 0, 0 }, // boss trigger 4
    {   298,   -345,   -385,   400,   400, 2, 0, 0 }, // boss trigger 3
    {     3,    416,   -278,    12,    18, 2, 0, 0 }, // create trigger 2
    {     3,    547,   -277,    12,    18, 2, 0, 0 }, // create trigger 3
    {     3,    554,   -132,    12,    18, 2, 0, 0 }, // create trigger 4
    {   729,   2325,    500,   281,   204, 2, 0, 0 }, // Active 5
    {   729,   2905,    505,   281,   204, 2, 0, 0 }, // blocking
    {   729,    870,    495,   281,   204, 2, 0, 0 }, // freddy hit
    {     3,    766,   -280,    12,    18, 2, 0, 0 }, // spin
    {     3,    955,   -220,    12,    18, 2, 0, 0 }, // character
    {     3,    955,   -202,    12,    18, 2, 0, 0 }, // Monty
    {     3,    955,   -184,    12,    18, 2, 0, 0 }, // Roxy
    {     3,    955,   -166,    12,    18, 2, 0, 0 }, // bonus power
    {   463,   -700,   -295,   354,   366, 2, 1, 0 }, // special power
    {   270,   1450,   -515,   400,   400, 2, 0, 0 }, // reached end
    {   764,   -424,    482,   232,   232, 2, 0, 0 }, // crate
    {     3,   -350,    325,    12,    18, 2, 0, 0 }, // crate
    {     3,    -58,    193,    12,    18, 2, 0, 0 }, // crate
    {   213,   -275,    785,   157,   100, 2, 0, 0 }, // beat level
    {  1029,    712,     58,   850,   750, 2, 0, 0 }, // Vanny
    {     1,    995,    525,    83,    83, 2, 0, 0 }, // fire girl
    {     3,   -100,    260,    12,    18, 2, 0, 0 }, // trap
    {     3,   -125,    305,    12,    18, 2, 0, 0 }, // IceFreddy
    {   729,   3495,    505,   281,   204, 2, 0, 0 }, // block it
    {   128,    -54,     48,    20,    39, 3, 0, 2 }, // Backdrop
    {   128,    -54,     87,    20,    39, 3, 0, 2 }, // Backdrop
    {     3,   1064,    -82,    12,    18, 3, 0, 1 }, // Quick Backdrop 2
    {     3,   1007,   -117,    12,    18, 3, 0, 1 }, // Quick Backdrop 10
    {     3,   1146,   -158,    12,    18, 3, 0, 1 }, // Quick Backdrop 8
    {     3,   1146,   -140,    12,    18, 3, 0, 1 }, // Quick Backdrop 11
    {   776,      0,   4320,  3840,  1080, 3, 0, 0 }, // Chica
    {   776,  -1470,   4320,  3840,  1080, 3, 0, 0 }, // sparkle
    {   362,     49,    113,    50,    50, 3, 0, 0 }, // play music
    {   462,     53,    168,   102,    18, 3, 0, 0 }, // hard mode?
    {   205,  -2475,   -255,    17,    62, 3, 0, 0 }, // Layer object
    {   828,     50,     34,    32,    62, 3, 0, 0 }, // snack
    {     3,   1064,   -172,    12,    18, 3, 0, 0 }, // giant animdude
    {     3,   1064,   -154,    12,    18, 3, 0, 0 }, // walk target 2
    {     3,   1064,   -136,    12,    18, 3, 0, 0 }, // faulty
    {     3,   1064,   -118,    12,    18, 3, 0, 0 }, // been played
    {     3,   1064,   -100,    12,    18, 3, 0, 0 }, // crate
    {   128,    -54,    126,    20,    39, 3, 0, 0 }, // Lives 5
    {   273,   1891,   1050,    37,    86, 3, 0, 0 }, // Active 7
    {   358,   1882,   1064,    10,    19, 3, 0, 0 }, // Lives 4
    {   827,     50,     34,    32,    62, 3, 0, 0 }, // Lives 2
    {   826,     44,    107,    50,    50, 3, 0, 0 }, // PoisonFoxy
    {     3,   -213,    168,    12,    18, 3, 0, 0 }, // Thug Chica
    {   526,   -444,     31,   288,   368, 3, 1, 0 }, //  Thuglife Chipper
    {   213,   -545,   -105,   157,   100, 3, 0, 0 }, // Punk Mangle
    {     3,   -105,    260,    12,    18, 3, 0, 0 }, // mech
    {     3,    625,    -45,    12,    18, 3, 0, 0 }, // Marionette
    {     3,    965,    -60,    12,    18, 3, 0, 0 }, // Mechionette
    {     3,    965,    -42,    12,    18, 3, 0, 0 }, // H sisters
    {   843,   1269,     92,   120,    18, 4, 0, 0 }, // Clown
    {   843,   1265,     92,   120,    18, 4, 0, 0 }, // test1
    {   843,   1263,     90,   120,    18, 4, 0, 0 }, // test2
    {   843,   1262,     86,   120,    18, 4, 0, 0 }, // test3
    {   843,   1265,     86,   120,    18, 4, 0, 0 }, // score
    {   875,   1257,     39,   618,    42, 4, 0, 0 }, // score 2
    {   213,    -15,   -240,   157,   100, 4, 0, 0 }, // Lives
    {   497,    525,    415,   812,   162, 4, 0, 0 }, // Lives 3
    {   728,    325,    210,  1236,   175, 4, 0, 0 }, // what level
    {   730,      0,      0,    32,    32, 4, 0, 0 }, // charge back
    {     3,   1222,    -42,    12,    18, 4, 0, 0 }, // charge special
    {     3,    672,    -32,    12,    18, 4, 0, 0 }, // block fx
    {     3,    692,    -67,    12,    18, 4, 0, 0 }, // snack
    {   128,    565,    -71,    20,    39, 4, 0, 0 }, // exp sound
    {     3,   -180,    680,    12,    18, 4, 0, 0 }, // reset
    {   876,     40,    891,   138,   138, 4, 0, 0 }, // test 1
    {   184,     50,   1029,   146,    28, 4, 0, 0 }, // test 2
    {   850,    188,   1021,    18,    35, 4, 0, 0 }, // boss name 2
    {   843,   1266,     89,   120,    18, 4, 0, 0 }, // boss name 2
    {   846,   1392,   -149,   142,    19, 4, 0, 0 }, // boss name 2
    {   847,   1400,   -115,   131,    23, 4, 0, 0 }, // boss name 2
    {   848,   1423,    -78,    74,    23, 4, 0, 0 }, // boss name 2
    {   866,   1421,    -42,   145,    18, 4, 0, 0 }, // Active 15
    {   249,   1805,   1020,    76,    25, 4, 0, 0 }, // snack
    {   830,   1467,    400,   346,   256, 4, 0, 0 }, // Active 8
    {   736,      0,      0,  1920,  1080, 5, 0, 0 }, // Active 11
    {   736,      0,      0,  1920,  1080, 5, 0, 0 }, // make at 00
    {   736,      0,      0,  1920,  1080, 5, 0, 0 }, // character level
};

static const int kFuryMapInstCount = (int)(sizeof(kFuryMapInsts)/sizeof(kFuryMapInsts[0]));

} // namespace fnaf

#endif // FNAF_FURYRAGE_MAP_H
