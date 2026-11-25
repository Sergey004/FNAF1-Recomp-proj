/**
 * FNaF3Office.h — GENERATED data, do not edit by hand.
 * The four office sprites whose motion is baked into their animation
 * cells: the run-past dash, the Golden Freddy walk and the two
 * phantom faces. Every cell carries its own size and hotspot, so
 * the draw origin is stored per cell (the parent's instance point
 * plus the child's offset, minus the cell's hotspot).
 * VS2010/C++03.
 */
#ifndef FNAF_FNAF3OFFICE_H
#define FNAF_FNAF3OFFICE_H

namespace fnaf {

struct FNaF3OfficeCell {
    unsigned short img; short left, top;
    unsigned short w, h;
};

static const FNaF3OfficeCell RunPastA0[16] = {
    {   207,   1351,   -353,   250,   768 },
    {   208,   1351,   -353,   250,   768 },
    {   209,   1351,   -353,   250,   768 },
    {   211,   1351,   -353,   250,   768 },
    {   212,   1351,   -353,   250,   768 },
    {   213,   1351,   -353,   250,   768 },
    {   214,   1351,   -353,   250,   768 },
    {   217,   1351,   -353,   250,   768 },
    {   218,   1351,   -353,   250,   768 },
    {   219,   1351,   -353,   250,   768 },
    {   220,   1351,   -353,   250,   768 },
    {   221,   1351,   -353,   250,   768 },
    {   222,   1351,   -353,   250,   768 },
    {   223,   1351,   -353,   250,   768 },
    {   224,   1351,   -353,   250,   768 },
    {   244,   1351,   -353,   250,   768 },
};
static const int RunPastA0Count = 16;
static const float RunPastA0Fps = 30.0f;

static const FNaF3OfficeCell FWalkA0[14] = {
    {   653,   1476,    414,   310,   413 },
    {   655,   1476,    414,   310,   413 },
    {   641,   1476,    414,   310,   413 },
    {   642,   1476,    414,   310,   413 },
    {   643,   1476,    414,   310,   413 },
    {   644,   1476,    414,   310,   413 },
    {   645,   1476,    414,   310,   413 },
    {   646,   1476,    414,   310,   413 },
    {   647,   1476,    414,   310,   413 },
    {   648,   1476,    414,   310,   413 },
    {   649,   1476,    414,   310,   413 },
    {   650,   1476,    414,   310,   413 },
    {   651,   1476,    414,   310,   413 },
    {   652,   1476,    414,   310,   413 },
};
static const int FWalkA0Count = 14;
static const float FWalkA0Fps = 18.0f;

static const FNaF3OfficeCell FWalkA12[11] = {
    {   662,   1476,    414,   310,   413 },
    {   635,   1476,    414,   310,   413 },
    {   664,   1476,    414,   310,   413 },
    {   639,   1476,    414,   310,   413 },
    {   640,   1476,    414,   310,   413 },
    {   654,   1476,    414,   310,   413 },
    {   656,   1476,    414,   310,   413 },
    {   657,   1476,    414,   310,   413 },
    {   658,   1476,    414,   310,   413 },
    {   659,   1476,    414,   310,   413 },
    {   660,   1476,    414,   310,   413 },
};
static const int FWalkA12Count = 11;
static const float FWalkA12Fps = 24.0f;

static const FNaF3OfficeCell Chica2A0[1] = {
    {   399,   1486,    564,   646,   654 },
};
static const int Chica2A0Count = 1;
static const float Chica2A0Fps = 30.0f;

static const FNaF3OfficeCell Chica2A12[15] = {
    {   428,   1465,    559,   680,   659 },
    {   432,   1416,    548,   747,   670 },
    {   434,   1365,    534,   811,   684 },
    {   435,   1344,    517,   838,   701 },
    {   439,   1330,    499,   855,   719 },
    {   441,   1319,    485,   883,   733 },
    {   459,   1313,    473,   907,   745 },
    {   461,   1310,    462,   898,   756 },
    {   462,   1307,    450,   907,   768 },
    {   463,   1306,    450,   913,   768 },
    {   464,   1304,    450,   933,   768 },
    {   465,   1303,    450,   952,   768 },
    {   466,   1303,    450,   967,   768 },
    {   467,   1311,    450,   968,   768 },
    {   468,   1314,    450,   970,   768 },
};
static const int Chica2A12Count = 15;
static const float Chica2A12Fps = 36.0f;

static const FNaF3OfficeCell FoxyA0[1] = {
    {   302,   1128,   -258,   313,   709 },
};
static const int FoxyA0Count = 1;
static const float FoxyA0Fps = 30.0f;

static const FNaF3OfficeCell FoxyA12[13] = {
    {   184,   1136,   -257,   313,   709 },
    {   183,   1119,   -218,   340,   670 },
    {   185,   1111,    -98,   361,   550 },
    {   170,   1087,    -47,   380,   499 },
    {   186,   1095,    -67,   395,   519 },
    {   174,   1120,   -146,   417,   598 },
    {   175,   1127,   -243,   465,   598 },
    {   176,   1046,   -316,   619,   639 },
    {   177,    897,   -316,   854,   604 },
    {   178,    761,   -316,  1078,   670 },
    {   179,    637,   -316,  1314,   768 },
    {   180,    637,   -316,  1469,   768 },
    {   181,    637,   -316,  1506,   768 },
};
static const int FoxyA12Count = 13;
static const float FoxyA12Fps = 24.0f;

} // namespace fnaf

#endif // FNAF_FNA3OFFICE_H
