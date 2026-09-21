////////////////////////////////////////////////////////////////////
//
// FNAF1-Recomp-proj.spa.h
//
// Auto-generated on Monday, 21 September 2026 at 14:05:13
// Xbox LIVE Game Config project version 1.0.37.0
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


#ifdef __cplusplus
}
#endif

#endif // __FIVE_NIGHTS_AT_FREDDY_SPA_H__


