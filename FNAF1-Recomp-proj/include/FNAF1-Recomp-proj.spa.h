////////////////////////////////////////////////////////////////////
//
// FNAF1-Recomp-proj.spa.h
//
// Auto-generated on Sunday, 25 October 2026 at 19:41:12
// Xbox LIVE Game Config project version 1.0.50.0
// SPA Compiler version 1.0.0.0
//
////////////////////////////////////////////////////////////////////

#ifndef __FIVE_NIGHTS_AT_FREDDY_SPA_H__
#define __FIVE_NIGHTS_AT_FREDDY_SPA_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Title info
//

#define TITLEID_FIVE_NIGHTS_AT_FREDDY               0x464E4131

//
// Context ids
//
// These values are passed as the dwContextId to XUserSetContext.
//


//
// Context values
//
// These values are passed as the dwContextValue to XUserSetContext.
//

// Values for X_CONTEXT_PRESENCE

#define CONTEXT_PRESENCE_IN_MENU                    0

// Values for X_CONTEXT_GAME_MODE


//
// Property ids
//
// These values are passed as the dwPropertyId value to XUserSetProperty
// and as the dwPropertyId value in the XUSER_PROPERTY structure.
//


//
// Achievement ids
//
// These values are used in the dwAchievementId member of the
// XUSER_ACHIEVEMENT structure that is used with
// XUserWriteAchievements and XUserCreateAchievementEnumerator.
//

#define ACHIEVEMENT_ONE_NIGHT_AT_FREDDYS            1
#define ACHIEVEMENT_TWO_NIGHTS_AT_FREDDYS           2
#define ACHIEVEMENT_THREE_NIGHTS_AT_FREDDYS         3
#define ACHIEVEMENT_FOUR_NIGHTS_AT_FREDDYS          4
#define ACHIEVEMENT_FIVE_NIGHTS_AT_FREDDYS          5
#define ACHIEVEMENT_OVERTIME                        6
#define ACHIEVEMENT_NO_TAMPERING                    7
#define ACHIEVEMENT_NO_RUNNING                      8
#define ACHIEVEMENT_NO_LAUGHING                     9
#define ACHIEVEMENT_NO_HIDING                       10
#define ACHIEVEMENT_F2_ONE_NIGHT                    11
#define ACHIEVEMENT_F2_TWO_NIGHTS                   12
#define ACHIEVEMENT_F2_THREE_NIGHTS                 13
#define ACHIEVEMENT_F2_FOUR_NIGHTS                  14
#define ACHIEVEMENT_F2_FIVE_NIGHTS                  15
#define ACHIEVEMENT_F2_YOU_TAMPERED                 16
#define ACHIEVEMENT_F2_YOU_SOOTHED                  17
#define ACHIEVEMENT_F2_YOU_STAYED                   18
#define ACHIEVEMENT_F2_YOU_TRIED                    19
#define ACHIEVEMENT_F2_YOU_FAILED                   20
#define ACHIEVEMENT_F3_ONE_NIGHT                    21
#define ACHIEVEMENT_F3_TWO_NIGHTS                   22
#define ACHIEVEMENT_F3_THREE_NIGHTS                 23
#define ACHIEVEMENT_F3_FOUR_NIGHTS                  24
#define ACHIEVEMENT_F3_FIVE_NIGHTS                  25
#define ACHIEVEMENT_F3_NIGHTMARE                    26
#define ACHIEVEMENT_F3_AGGRESSIVE_NIGHTMARE         27
#define ACHIEVEMENT_F3_TRAPPED                      28
#define ACHIEVEMENT_F3_THE_END                      29
#define ACHIEVEMENT_F3_TRUE_END                     30
#define ACHIEVEMENT_F4_ONE_NIGHT                    31
#define ACHIEVEMENT_F4_TWO_NIGHTS                   32
#define ACHIEVEMENT_F4_THREE_NIGHTS                 33
#define ACHIEVEMENT_F4_FOUR_NIGHTS                  34
#define ACHIEVEMENT_F4_FIVE_NIGHTS                  35
#define ACHIEVEMENT_F4_6TH_NIGHT                    36
#define ACHIEVEMENT_F4_NIGHTMARE                    37
#define ACHIEVEMENT_F4_20_20_20_20                  38
#define ACHIEVEMENT_F4_FUN                          39
#define ACHIEVEMENT_F4_TOO_BAD                      40
#define ACHIEVEMENT_SL_CONTROLLED_SHOCK             41
#define ACHIEVEMENT_SL_BREAKER_ROOM                 42
#define ACHIEVEMENT_SL_PARTS_AND_SERVICE            43
#define ACHIEVEMENT_SL_SPRING_LOCK_MAINTENANCE      44
#define ACHIEVEMENT_SL_ICE_CREAM                    45
#define ACHIEVEMENT_SL_MEET_THE_CAST                46
#define ACHIEVEMENT_SL_LOOK_LIKE_YOU                47
#define ACHIEVEMENT_SL_EXOTIC_BUTTERS               48
#define ACHIEVEMENT_SL_YOU_WONT_DIE                 49
#define ACHIEVEMENT_SL_FAZBEARS_FRIGHT              50

//
// AvatarAssetAward ids
//


//
// Stats view ids
//
// These are used in the dwViewId member of the XUSER_STATS_SPEC structure
// passed to the XUserReadStats* and XUserCreateStatsEnumerator* functions.
//

// Skill leaderboards for ranked game modes


// Skill leaderboards for unranked (standard) game modes


// Title defined leaderboards


//
// Stats view column ids
//
// These ids are used to read columns of stats views.  They are specified in
// the rgwColumnIds array of the XUSER_STATS_SPEC structure.  Rank, rating
// and gamertag are not retrieved as custom columns and so are not included
// in the following definitions.  They can be retrieved from each row's
// header (e.g., pStatsResults->pViews[x].pRows[y].dwRank, etc.).
//

//
// Matchmaking queries
//
// These values are passed as the dwProcedureIndex parameter to
// XSessionSearch to indicate which matchmaking query to run.
//


//
// Gamer pictures
//
// These ids are passed as the dwPictureId parameter to XUserAwardGamerTile.
//


//
// Strings
//
// These ids are passed as the dwStringId parameter to XReadStringsFromSpaFile.
//

#define SPASTRING_ONE_NIGHT_AT_FREDDYS_NAME         2
#define SPASTRING_ONE_NIGHT_AT_FREDDYS_DESC         3
#define SPASTRING_SURVIVE_YOUR_FIRST_NIGHT_ON_THE_JOB 4
#define SPASTRING_TWO_NIGHTS_AT_FREDDYS_NAME        5
#define SPASTRING_TWO_NIGHTS_AT_FREDDYS_DESC        6
#define SPASTRING_THREE_NIGHTS_AT_FREDDYS_NAME      7
#define SPASTRING_THREE_NIGHTS_AT_FREDDYS_DESC      8
#define SPASTRING_SURVIVE_A_THIRD_NIGHT             9
#define SPASTRING_FOUR_NIGHTS_AT_FREDDYS_NAME       10
#define SPASTRING_FOUR_NIGHTS_AT_FREDDYS_DESC       11
#define SPASTRING_SURVIVE_A_FOURTH_NIGHT            12
#define SPASTRING_IN_MENU                           13
#define SPASTRING_FIVE_NIGHTS_AT_FREDDYS_NAME       14
#define SPASTRING_FIVE_NIGHTS_AT_FREDDYS_DESC       15
#define SPASTRING_SURVIVE_ALL_FIVE_NIGHTS           16
#define SPASTRING_SURVIVE_A_SECOND_NIGHT            17
#define SPASTRING_OVERTIME_NAME                     18
#define SPASTRING_OVERTIME_DESC                     19
#define SPASTRING_SURVIVE_THE_SIXTH_NIGHT           20
#define SPASTRING_NO_TAMPERING_NAME                 21
#define SPASTRING_NO_TAMPERING_DESC                 22
#define SPASTRING_COMPLETE_CUSTOM_NIGHT_WITH_AI_SET_TO_ALL_20 23
#define SPASTRING_NO_RUNNING_NAME                   24
#define SPASTRING_NO_RUNNING_DESC                   25
#define SPASTRING_PREVENT_FOXY_FROM_LEAVING_PIRATE_COVE_ON_NIGHT_4 26
#define SPASTRING_NO_LAUGHING_NAME                  27
#define SPASTRING_NO_LAUGHING_DESC                  28
#define SPASTRING_KEEP_FREDDY_FROM_REACHING_THE_EAST_HALL_ON_NIGHT_5 29
#define SPASTRING_NO_HIDING_NAME                    30
#define SPASTRING_NO_HIDING_DESC                    31
#define SPASTRING_GET_CAUGHT_BY_AN_ANIMATRONIC      32
#define SPASTRING_F2_ONE_NIGHT_NAME                 33
#define SPASTRING_SURVIVE_THE_1ST_NIGHT_            34
#define SPASTRING_F2_TWO_NIGHTS_NAME                35
#define SPASTRING_SURVIVE_THE_2ND_NIGHT_            36
#define SPASTRING_F2_THREE_NIGHTS_NAME              37
#define SPASTRING_SURVIVE_THE_3RD_NIGHT_            38
#define SPASTRING_F2_FOUR_NIGHTS_NAME               39
#define SPASTRING_SURVIVE_THE_4TH_NIGHT_            40
#define SPASTRING_F2_FIVE_NIGHTS_NAME               41
#define SPASTRING_SURVIVE_THE_5TH_NIGHT_            42
#define SPASTRING_F2_YOU_TAMPERED_NAME              43
#define SPASTRING_COMPLETE_THE_CUSTOM_NIGHT_WITH_ALL_AIS_SET_TO_20_ 44
#define SPASTRING_F2_YOU_SOOTHED_NAME               45
#define SPASTRING_KEEP_THE_MUSIC_BOX_WOUND_ABOVE_50__FOR_ALL_OF_NIGHT_4_ 46
#define SPASTRING_F2_YOU_STAYED_NAME                47
#define SPASTRING_SURVIVE_THE_6TH_NIGHT_            48
#define SPASTRING_F2_YOU_TRIED_NAME                 49
#define SPASTRING_PLAY_ALL_THE_MINIGAMES_           50
#define SPASTRING_F2_YOU_FAILED_NAME                51
#define SPASTRING_EXPERIENCE_A_JUMPSCARE__AND_THEN____SOMETHING_MORE_ 52
#define SPASTRING_F3_ONE_NIGHT_NAME                 53
#define SPASTRING_F3_SURVIVE_THE_1ST_NIGHT_         54
#define SPASTRING_F3_TWO_NIGHTS_NAME                55
#define SPASTRING_F3_SURVIVE_THE_2ND_NIGHT_         56
#define SPASTRING_F3_THREE_NIGHTS_NAME              57
#define SPASTRING_F3_SURVIVE_THE_3RD_NIGHT_         58
#define SPASTRING_F3_FOUR_NIGHTS_NAME               59
#define SPASTRING_F3_SURVIVE_THE_4TH_NIGHT_         60
#define SPASTRING_F3_FIVE_NIGHTS_NAME               61
#define SPASTRING_F3_SURVIVE_THE_5TH_NIGHT_         62
#define SPASTRING_F3_NIGHTMARE_NAME                 63
#define SPASTRING_F3_SURVIVE_NIGHTMARE_MODE_        64
#define SPASTRING_F3_AGGRESSIVE_NIGHTMARE_NAME      65
#define SPASTRING_F3_SURVIVE_AGGRESSIVE_NIGHTMARE_MODE_ 66
#define SPASTRING_F3_TRAPPED_NAME                   67
#define SPASTRING_F3_LET_IT_REACH_YOUR_OFFICE_      68
#define SPASTRING_F3_THE_END_NAME                   69
#define SPASTRING_F3_FINISH_YOUR_FIRST_WEEK_S_SHIFT_ 70
#define SPASTRING_F3_TRUE_END_NAME                  71
#define SPASTRING_F3_DELIVER_CAKE_TO_CHILDREN_      72
#define SPASTRING_F4_ONE_NIGHT_NAME                 73
#define SPASTRING_F4_SURVIVE_THE_1ST_NIGHT_         74
#define SPASTRING_F4_TWO_NIGHTS_NAME                75
#define SPASTRING_F4_SURVIVE_THE_2ND_NIGHT_         76
#define SPASTRING_F4_THREE_NIGHTS_NAME              77
#define SPASTRING_F4_SURVIVE_THE_3RD_NIGHT_         78
#define SPASTRING_F4_FOUR_NIGHTS_NAME               79
#define SPASTRING_F4_SURVIVE_THE_4TH_NIGHT_         80
#define SPASTRING_F4_FIVE_NIGHTS_NAME               81
#define SPASTRING_F4_SURVIVE_THE_5TH_NIGHT_         82
#define SPASTRING_F4_6TH_NIGHT_NAME                 83
#define SPASTRING_F4_SURVIVE_THE_6TH_NIGHT_         84
#define SPASTRING_F4_NIGHTMARE_NAME                 85
#define SPASTRING_F4_SURVIVE_NIGHTMARE_MODE_        86
#define SPASTRING_F4_20_20_20_20_NAME               87
#define SPASTRING_F4_SURVIVE_20_20_20_20_MODE_      88
#define SPASTRING_F4_FUN_NAME                       89
#define SPASTRING_F4_WIN_ALL_MINIGAMES_             90
#define SPASTRING_F4_TOO_BAD_NAME                   91
#define SPASTRING_F4_DON_T_LISTEN_CAREFULLY_        92
#define SPASTRING_SL_CONTROLLED_SHOCK_NAME          93
#define SPASTRING_SL_COMPLETE_YOUR_FIRST_NIGHT_SHIFT_ 94
#define SPASTRING_SL_BREAKER_ROOM_NAME              95
#define SPASTRING_SL_COMPLETE_YOUR_SECOND_NIGHT_SHIFT_ 96
#define SPASTRING_SL_PARTS_AND_SERVICE_NAME         97
#define SPASTRING_SL_COMPLETE_YOUR_THIRD_NIGHT_SHIFT_ 98
#define SPASTRING_SL_SPRING_LOCK_MAINTENANCE_NAME   99
#define SPASTRING_SL_COMPLETE_YOUR_FOURTH_NIGHT_SHIFT_ 100
#define SPASTRING_SL_ICE_CREAM_NAME                 101
#define SPASTRING_SL_GIVE_ICE_CREAM_TO_A_CHILD_     102
#define SPASTRING_SL_MEET_THE_CAST_NAME             103
#define SPASTRING_SL_EXPERIENCE_ALL_DEATHS_AND_JUMPSCARES_ 104
#define SPASTRING_SL_LOOK_LIKE_YOU_NAME             105
#define SPASTRING_SL_DO_AS_YOU_RE_TOLD_             106
#define SPASTRING_SL_EXOTIC_BUTTERS_NAME            107
#define SPASTRING_SL_SURVIVE_UNTIL_MORNING_         108
#define SPASTRING_SL_YOU_WONT_DIE_NAME              109
#define SPASTRING_SL_COMPLETE_THE_CUSTOM_NIGHT_CHALLENGES_ 110
#define SPASTRING_SL_FAZBEARS_FRIGHT_NAME           111
#define SPASTRING_SL_COMPLETE_GOLDEN_FREDDY_MODE_   112


#ifdef __cplusplus
}
#endif

#endif // __FIVE_NIGHTS_AT_FREDDY_SPA_H__


