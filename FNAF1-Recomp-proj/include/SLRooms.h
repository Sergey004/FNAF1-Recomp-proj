/**
 * SLRooms.h - GENERATED data, do not edit by hand.
 * The Sister Location rooms as the dump draws them: the sprites
 * the frame-start state leaves visible and the animated objects
 * with every cell's own size and hotspot. The coordinates are in
 * the room's own space (the dump's world); the renderer scales
 * the room to the 1280x720 window (the height-squash convention).
 * VS2010/C++03.
 */
#ifndef FNAF_SLROOMS_H
#define FNAF_SLROOMS_H

namespace fnaf {

struct SLRoomSprite {
    unsigned short img; short x, y;
    unsigned short w, h;
};

struct SLRoomAnimCell {
    unsigned short img; short x, y;
    unsigned short w, h;
};

struct SLRoomAnim {
    const SLRoomAnimCell* cells; int count; float fps;
};

// frame 2: "Elevator" (1900x1000)
static const int SLElevRoomW = 1900;
static const int SLElevRoomH = 1000;
static const SLRoomSprite SLElevSprites[] = {
    {    69,    103,    990,  1700,  1000 },
    {    69,     98,      8,  1700,  1000 },
    {    80,    739,    450,    25,    27 },
    {   376,    551,    322,   178,    35 },
    {   359,     25,    640,   135,    65 },
};
static const int SLElevSpriteCount = 5;
static const SLRoomAnimCell SLElevAnim0[] = {
    {   819,      0,      0,  1900,  1000 },
    {   810,      0,      0,  1900,  1000 },
    {   823,      0,      0,  1900,  1000 },
    {   813,      0,      0,  1900,  1000 },
    {   814,      0,      0,  1900,  1000 },
    {   815,      0,      0,  1900,  1000 },
    {   816,      0,      0,  1900,  1000 },
    {   817,      0,      0,  1900,  1000 },
    {   818,      0,      0,  1900,  1000 },
    {   820,      0,      0,  1900,  1000 },
};
static const SLRoomAnim SLElevAnims[] = {
    { SLElevAnim0, 10, 54.0f },
};
static const int SLElevAnimCount = 1;
static const int SLElevAnimActive = 0;

// frame 3: "Vent crawl" (1700x1000)
static const int SLVentRoomW = 1700;
static const int SLVentRoomH = 1000;
static const SLRoomSprite SLVentSprites[] = {
    {   357,     24,    554,   191,   148 },
    {   359,   1114,    640,   135,    65 },
};
static const int SLVentSpriteCount = 2;
static const SLRoomAnimCell SLVentAnim0[] = {
    {    18,      0,      0,  1700,  1000 },
    {    50,      0,      0,  1700,  1000 },
    {    73,      0,      0,  1700,  1000 },
    {    75,      0,      0,  1700,  1000 },
    {    76,      0,      0,  1700,  1000 },
    {    77,      0,      0,  1700,  1000 },
    {    79,      0,      0,  1700,  1000 },
    {    81,      0,      0,  1700,  1000 },
    {    82,      0,      0,  1700,  1000 },
    {    83,      0,      0,  1700,  1000 },
    {    84,      0,      0,  1700,  1000 },
    {    85,      0,      0,  1700,  1000 },
    {    96,      0,      0,  1700,  1000 },
    {   101,      0,      0,  1700,  1000 },
    {   102,      0,      0,  1700,  1000 },
    {   103,      0,      0,  1700,  1000 },
};
static const SLRoomAnim SLVentAnims[] = {
    { SLVentAnim0, 16, 30.0f },
};
static const int SLVentAnimCount = 1;
static const int SLVentAnimActive = 0;

// frame 5: "Main Hub" (1900x1000)
static const int SLHubRoomW = 1900;
static const int SLHubRoomH = 1000;
static const SLRoomSprite SLHubSprites[] = {
    {    56,    256,     32,    32,    32 },
    {    56,    416,     96,    32,    32 },
    {    56,    704,    128,    32,    32 },
    {    56,    832,     96,    32,    32 },
    {    56,     32,    -32,    32,    32 },
    {    56,    160,    -32,    32,    32 },
    {    56,    256,    -32,    32,    32 },
    {    56,    384,    -32,    32,    32 },
    {  2059,   1393,    534,   319,   464 },
    {  2064,    158,    537,   319,   464 },
    {  2066,    837,    773,   220,   124 },
    {  1308,    909,    246,    77,   142 },
    {   380,    384,    544,   257,   600 },
    {   382,   1248,    544,   257,   600 },
    {    55,    832,    448,    30,    30 },
    {   300,      0,      0,  1280,   720 },
    {   359,     24,    644,   135,    65 },
};
static const int SLHubSpriteCount = 17;
static const SLRoomAnimCell SLHubAnim0[] = {
    {  2085,      0,      0,  1900,  1000 },
    {  2071,      0,      0,  1900,  1000 },
    {  2087,      0,      0,  1900,  1000 },
    {  2072,      0,      0,  1900,  1000 },
    {  2073,      0,      0,  1900,  1000 },
    {  2074,      0,      0,  1900,  1000 },
    {  2075,      0,      0,  1900,  1000 },
    {  2076,      0,      0,  1900,  1000 },
    {  2077,      0,      0,  1900,  1000 },
    {  2078,      0,      0,  1900,  1000 },
    {  2079,      0,      0,  1900,  1000 },
    {  2080,      0,      0,  1900,  1000 },
    {  2081,      0,      0,  1900,  1000 },
    {  2082,      0,      0,  1900,  1000 },
    {  2083,      0,      0,  1900,  1000 },
    {  2084,      0,      0,  1900,  1000 },
};
static const SLRoomAnim SLHubAnims[] = {
    { SLHubAnim0, 16, 30.0f },
};
static const int SLHubAnimCount = 1;
static const int SLHubAnimActive = 0;

// frame 6: "Baby's Room" (1700x900)
static const int SLBabyRoomW = 1700;
static const int SLBabyRoomH = 900;
static const SLRoomSprite SLBabySprites[] = {
    {    56,    256,     32,    32,    32 },
    {    56,    416,     96,    32,    32 },
    {    56,    704,    128,    32,    32 },
    {    56,    832,     96,    32,    32 },
    {    22,   1120,    480,   257,   600 },
    {    55,    832,    448,    30,    30 },
    {   300,      0,      0,  1280,   720 },
    {   304,      0,      0,  1280,   720 },
    {   359,     30,    642,   135,    65 },
    {   361,     37,    643,    49,    65 },
};
static const int SLBabySpriteCount = 10;
static const SLRoomAnimCell SLBabyAnim0[] = {
    {   829,     -1,     -1,  1700,  1000 },
    {   831,     -1,     -1,  1700,  1000 },
    {   832,     -1,     -1,  1700,  1000 },
    {   821,     -1,     -1,  1700,  1000 },
    {   833,     -1,     -1,  1700,  1000 },
    {   822,     -1,     -1,  1700,  1000 },
    {   824,     -1,     -1,  1700,  1000 },
    {   825,     -1,     -1,  1700,  1000 },
    {   826,     -1,     -1,  1700,  1000 },
    {   827,     -1,     -1,  1700,  1000 },
    {   828,     -1,     -1,  1700,  1000 },
};
static const SLRoomAnim SLBabyAnims[] = {
    { SLBabyAnim0, 11, 30.0f },
};
static const int SLBabyAnimCount = 1;
static const int SLBabyAnimActive = 0;

// frame 8: "Ballora Gallery" (2200x1100)
static const int SLBalloraRoomW = 2200;
static const int SLBalloraRoomH = 1100;
static const SLRoomSprite SLBalloraSprites[] = {
    {    80,    800,    480,    25,    27 },
    {   357,     19,    554,   191,   148 },
    {   359,   1120,    640,   135,    65 },
};
static const int SLBalloraSpriteCount = 3;
static const SLRoomAnimCell SLBalloraAnim0[] = {
    {  1955,      0,    768,  2200,   332 },
    {  1954,      0,    767,  2200,   333 },
    {  1261,      0,    765,  2200,   335 },
    {  1956,      0,    762,  2200,   338 },
    {  1534,      0,    759,  2200,   341 },
    {  1727,      0,    757,  2200,   343 },
    {  1728,      0,    759,  2200,   341 },
    {  1729,      0,    762,  2200,   338 },
    {  1950,      0,    765,  2200,   335 },
    {  1951,      0,    767,  2200,   333 },
    {  1952,      0,    768,  2200,   332 },
};
static const SLRoomAnimCell SLBalloraAnim1[] = {
    {   194,    394,   -197,  1300,   841 },
    {   195,    394,   -197,  1300,   841 },
    {   196,    394,   -197,  1300,   841 },
    {   197,    394,   -197,  1300,   841 },
    {   198,    394,   -197,  1300,   841 },
    {   199,    394,   -197,  1300,   841 },
    {   200,    394,   -197,  1300,   841 },
    {   201,    394,   -197,  1300,   841 },
    {   202,    394,   -197,  1300,   841 },
    {   203,    394,   -197,  1300,   841 },
    {   204,    394,   -197,  1300,   841 },
};
static const SLRoomAnimCell SLBalloraAnim2[] = {
    {   230,    392,    650,  1300,   841 },
    {   231,    392,    650,  1300,   841 },
    {   232,    392,    650,  1300,   841 },
    {   233,    392,    650,  1300,   841 },
    {   234,    392,    650,  1300,   841 },
    {   235,    392,    650,  1300,   841 },
    {   236,    392,    650,  1300,   841 },
    {   237,    392,    650,  1300,   841 },
    {   238,    392,    650,  1300,   841 },
    {   239,    392,    650,  1300,   841 },
    {   240,    392,    650,  1300,   841 },
};
static const SLRoomAnimCell SLBalloraAnim3[] = {
    {   518,   -125,   -699,   250,   700 },
    {   486,   -125,   -699,   250,   700 },
    {   520,   -125,   -699,   250,   700 },
    {   489,   -125,   -699,   250,   700 },
    {   490,   -125,   -699,   250,   700 },
    {   491,   -125,   -699,   250,   700 },
    {   492,   -125,   -699,   250,   700 },
    {   493,   -125,   -699,   250,   700 },
    {   494,   -125,   -699,   250,   700 },
    {   495,   -125,   -699,   250,   700 },
    {   496,   -125,   -699,   250,   700 },
    {   497,   -125,   -699,   250,   700 },
    {   498,   -125,   -699,   250,   700 },
    {   499,   -125,   -699,   250,   700 },
    {   500,   -125,   -699,   250,   700 },
    {   501,   -125,   -699,   250,   700 },
    {   502,   -125,   -699,   250,   700 },
    {   503,   -125,   -699,   250,   700 },
    {   504,   -125,   -699,   250,   700 },
    {   505,   -125,   -699,   250,   700 },
    {   506,   -125,   -699,   250,   700 },
    {   507,   -125,   -699,   250,   700 },
    {   508,   -125,   -699,   250,   700 },
    {   509,   -125,   -699,   250,   700 },
    {   510,   -125,   -699,   250,   700 },
    {   511,   -125,   -699,   250,   700 },
    {   512,   -125,   -699,   250,   700 },
    {   513,   -125,   -699,   250,   700 },
    {   514,   -125,   -699,   250,   700 },
    {   515,   -125,   -699,   250,   700 },
    {   516,   -125,   -699,   250,   700 },
    {   517,   -125,   -699,   250,   700 },
};
static const SLRoomAnim SLBalloraAnims[] = {
    { SLBalloraAnim0, 11, 15.0f },
    { SLBalloraAnim1, 11, 30.0f },
    { SLBalloraAnim2, 11, 30.0f },
    { SLBalloraAnim3, 32, 24.0f },
};
static const int SLBalloraAnimCount = 4;
static const int SLBalloraAnimFloor = 0;
static const int SLBalloraAnimDoor = 1;
static const int SLBalloraAnimDoor_2 = 2;
static const int SLBalloraAnimDancer = 3;

// frame 9: "Breaker Room" (1900x1000)
static const int SLBreakerRoomW = 1900;
static const int SLBreakerRoomH = 1000;
static const SLRoomSprite SLBreakerSprites[] = {
    {   279,     -9,      1,    20,    20 },
    {   280,    308,    509,   500,   500 },
    {   361,     36,    620,    49,    65 },
    {   469,     50,     50,  1175,   630 },
    {    55,    800,    448,    30,    30 },
    {   580,    340,    680,   557,    38 },
    {   586,     90,     90,   138,    43 },
    {   586,     90,    163,   138,    43 },
    {   586,     90,    236,   138,    43 },
    {   586,     90,    309,   138,    43 },
    {   586,     90,    382,   138,    43 },
    {   586,     90,    455,   138,    43 },
    {   586,     90,    528,   138,    43 },
    {   586,     90,    601,   138,    43 },
    {  1825,     90,     70,   177,    14 },
    {  1828,     90,    143,   178,    18 },
    {  1829,     91,    216,   123,    14 },
    {  1830,     91,    290,   123,    14 },
    {  1831,     92,    362,   123,    14 },
    {  1832,     92,    434,   113,    14 },
    {  1833,     92,    508,    74,    14 },
    {  1834,     92,    581,    74,    14 },
    {   577,    323,     73,   217,    37 },
    {   628,   1017,     64,   123,    14 },
    {   481,   1150,     57,    25,    25 },
    {   633,   1175,     57,    25,    25 },
    {   632,   1200,     57,    25,    25 },
};
static const int SLBreakerSpriteCount = 27;
static const SLRoomAnimCell SLBreakerAnim0[] = {
    {   594,      0,      0,  1900,  1000 },
    {   587,      0,      0,  1900,  1000 },
    {   596,      0,      0,  1900,  1000 },
    {   593,      0,      0,  1900,  1000 },
};
static const SLRoomAnimCell SLBreakerAnim1[] = {
    {   583,    659,    399,    44,    44 },
    {   584,    659,    399,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim2[] = {
    {   583,    933,    395,    44,    44 },
    {   584,    933,    395,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim3[] = {
    {   583,   1060,    120,    44,    44 },
    {   584,   1060,    120,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim4[] = {
    {   583,    475,    128,    44,    44 },
    {   584,    475,    128,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim5[] = {
    {   583,    794,    130,    44,    44 },
    {   584,    794,    130,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim6[] = {
    {   583,    798,    217,    44,    44 },
    {   584,    798,    217,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim7[] = {
    {   583,   1086,    393,    44,    44 },
    {   584,   1086,    393,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim8[] = {
    {   583,    797,    568,    44,    44 },
    {   584,    797,    568,    44,    44 },
};
static const SLRoomAnimCell SLBreakerAnim9[] = {
    {   635,   1011,     54,   214,    32 },
    {  1849,   1011,     54,   214,    32 },
};
static const SLRoomAnim SLBreakerAnims[] = {
    { SLBreakerAnim0, 4, 30.0f },
    { SLBreakerAnim1, 2, 3.0f },
    { SLBreakerAnim2, 2, 3.0f },
    { SLBreakerAnim3, 2, 3.0f },
    { SLBreakerAnim4, 2, 3.0f },
    { SLBreakerAnim5, 2, 3.0f },
    { SLBreakerAnim6, 2, 3.0f },
    { SLBreakerAnim7, 2, 3.0f },
    { SLBreakerAnim8, 2, 3.0f },
    { SLBreakerAnim9, 2, 15.0f },
};
static const int SLBreakerAnimCount = 10;
static const int SLBreakerAnimRoom = 0;
static const int SLBreakerAnimBallora_gall = 1;
static const int SLBreakerAnimFuntime_aud = 2;
static const int SLBreakerAnimObs2 = 3;
static const int SLBreakerAnimObs1 = 4;
static const int SLBreakerAnimCircus_aud = 5;
static const int SLBreakerAnimCirc_control = 6;
static const int SLBreakerAnimParts_and_s = 7;
static const int SLBreakerAnimElevators = 8;
static const int SLBreakerAnimActive_15 = 9;

// frame 10: "Funtime Auditorium" (1900x1100)
static const int SLFuntimeRoomW = 1900;
static const int SLFuntimeRoomH = 1100;
static const SLRoomSprite SLFuntimeSprites[] = {
    {   193,    235,    -39,  1400,   841 },
    {    80,    800,    480,    25,    27 },
    {   277,   1123,    636,   135,    68 },
    {   357,     19,    555,   191,   148 },
};
static const int SLFuntimeSpriteCount = 4;
static const SLRoomAnimCell SLFuntimeAnim0[] = {
    {   261,      0,    800,  1900,   300 },
    {   252,      0,    800,  1900,   300 },
    {   288,      0,    800,  1900,   300 },
    {   253,      0,    800,  1900,   300 },
    {   254,      0,    800,  1900,   300 },
    {   255,      0,    800,  1900,   300 },
    {   256,      0,    800,  1900,   300 },
    {   257,      0,    800,  1900,   300 },
    {   258,      0,    800,  1900,   300 },
    {   259,      0,    800,  1900,   300 },
};
static const SLRoomAnim SLFuntimeAnims[] = {
    { SLFuntimeAnim0, 10, 15.0f },
};
static const int SLFuntimeAnimCount = 1;
static const int SLFuntimeAnimFloor = 0;

} // namespace fnaf

#endif // FNAF_SLROOMS_H
