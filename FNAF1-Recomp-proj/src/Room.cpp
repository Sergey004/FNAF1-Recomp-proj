/*
 * Five Nights at Freddy's 1 — Recompilation
 * Room.cpp: Room definitions and map adjacency data
 */

#include "Room.h"
#include <cstring>

namespace fnaf {

// ============================================================
//  Forward declare static data arrays with explicit sizes
// ============================================================
static RoomInfo s_rooms[15];
static RoomConnection s_connections[33];
static i32 s_roomCount;
static i32 s_connectionCount;

// ============================================================
//  Room table initializer (called once)
// ============================================================

static void InitRoomData() {
    static const RoomInfo data[] = {
        { ROOM_SHOW_STAGE,       "Show Stage",        CAM_1A },
        { ROOM_DINING_AREA,      "Dining Area",       CAM_1B },
        { ROOM_PIRATE_COVE,      "Pirate Cove",       CAM_1C },
        { ROOM_WEST_HALL,        "West Hall",         CAM_2A },
        { ROOM_WEST_HALL_CORNER, "W. Hall Corner",    CAM_2B },
        { ROOM_SUPPLY_CLOSET,    "Supply Closet",     CAM_3  },
        { ROOM_EAST_HALL,        "East Hall",         CAM_4A },
        { ROOM_EAST_HALL_CORNER, "E. Hall Corner",    CAM_4B },
        { ROOM_BACKSTAGE,        "Backstage",         CAM_5  },
        { ROOM_KITCHEN,          "Kitchen",           CAM_6  },
        { ROOM_RESTROOMS,        "Restrooms",         CAM_7  },
        { ROOM_LEFT_DOOR,        "Left Door",         CAM_OFF },
        { ROOM_RIGHT_DOOR,       "Right Door",        CAM_OFF },
        { ROOM_OFFICE,           "Office",            CAM_OFF },
        { ROOM_NONE,             "None",              CAM_OFF }
    };
    for (i32 i = 0; i < 15; ++i) {
        s_rooms[i] = data[i];
    }
    s_roomCount = 15;

    static const RoomConnection connData[] = {
        { ROOM_SHOW_STAGE,   ROOM_DINING_AREA },
        { ROOM_DINING_AREA, ROOM_SHOW_STAGE },
        { ROOM_DINING_AREA, ROOM_BACKSTAGE },
        { ROOM_DINING_AREA, ROOM_PIRATE_COVE },
        { ROOM_DINING_AREA, ROOM_RESTROOMS },
        { ROOM_DINING_AREA, ROOM_EAST_HALL },
        { ROOM_DINING_AREA, ROOM_WEST_HALL },
        { ROOM_BACKSTAGE,    ROOM_DINING_AREA },
        { ROOM_PIRATE_COVE, ROOM_DINING_AREA },
        { ROOM_RESTROOMS,    ROOM_DINING_AREA },
        { ROOM_RESTROOMS,    ROOM_KITCHEN },
        { ROOM_RESTROOMS,    ROOM_EAST_HALL },
        { ROOM_KITCHEN,      ROOM_RESTROOMS },
        { ROOM_WEST_HALL,    ROOM_DINING_AREA },
        { ROOM_WEST_HALL,    ROOM_SUPPLY_CLOSET },
        { ROOM_WEST_HALL,    ROOM_WEST_HALL_CORNER },
        { ROOM_SUPPLY_CLOSET,ROOM_WEST_HALL },
        { ROOM_WEST_HALL_CORNER, ROOM_WEST_HALL },
        { ROOM_WEST_HALL_CORNER, ROOM_LEFT_DOOR },
        { ROOM_EAST_HALL,    ROOM_DINING_AREA },
        { ROOM_EAST_HALL,    ROOM_RESTROOMS },
        { ROOM_EAST_HALL,    ROOM_EAST_HALL_CORNER },
        { ROOM_EAST_HALL_CORNER, ROOM_EAST_HALL },
        { ROOM_EAST_HALL_CORNER, ROOM_RIGHT_DOOR },
        { ROOM_LEFT_DOOR,    ROOM_OFFICE },
        { ROOM_RIGHT_DOOR,   ROOM_OFFICE }
    };
    for (i32 i = 0; i < (i32)(sizeof(connData) / sizeof(connData[0])); ++i) {
        s_connections[i] = connData[i];
    }
    s_connectionCount = (i32)(sizeof(connData) / sizeof(connData[0]));
}

// ============================================================
//  Static initialization flag
// ============================================================
static bool s_initialized = false;

void RoomSystem::Initialize() {
    if (s_initialized) return;
    s_initialized = true;
    InitRoomData();
}


const RoomInfo& RoomSystem::GetRoomInfo(RoomId room) {
    if (!s_initialized) Initialize();
    for (i32 i = 0; i < s_roomCount; ++i) {
        if (s_rooms[i].id == room) {
            return s_rooms[i];
        }
    }
    if (s_roomCount <= 0) {
        // v2.7.13: deterministic fallback (also silences PREfast C6385)
        static const RoomInfo kFallback = { ROOM_NONE, "None", CAM_OFF };
        return kFallback;
    }
    return s_rooms[s_roomCount - 1]; // ROOM_NONE fallback
}

CameraId RoomSystem::GetRoomCamera(RoomId room) {
    if (!s_initialized) Initialize();
    for (i32 i = 0; i < s_roomCount; ++i) {
        if (s_rooms[i].id == room) return s_rooms[i].camera;
    }
    return CAM_OFF;
}

RoomId RoomSystem::GetCameraRoom(CameraId cam) {
    if (cam == CAM_OFF) return ROOM_NONE;
    if (!s_initialized) Initialize();
    for (i32 i = 0; i < s_roomCount; ++i) {
        if (s_rooms[i].camera == cam) return s_rooms[i].id;
    }
    return ROOM_NONE;
}

bool RoomSystem::AreConnected(RoomId a, RoomId b) {
    if (!s_initialized) Initialize();
    for (i32 i = 0; i < s_connectionCount; ++i) {
        const RoomConnection& c = s_connections[i];
        if ((c.from == a && c.to == b) || (c.from == b && c.to == a)) return true;
    }
    return false;
}

i32 RoomSystem::GetAdjacentRooms(RoomId room, RoomId* outRooms, i32 maxRooms) {
    if (!s_initialized) Initialize();
    i32 count = 0;
    for (i32 i = 0; i < s_connectionCount && count < maxRooms; ++i) {
        const RoomConnection& c = s_connections[i];
        if (c.from == room) outRooms[count++] = c.to;
        else if (c.to == room) outRooms[count++] = c.from;
    }
    return count;
}

bool RoomSystem::IsLeftSideRoom(RoomId room) {
    return room == ROOM_WEST_HALL || room == ROOM_WEST_HALL_CORNER ||
           room == ROOM_SUPPLY_CLOSET || room == ROOM_LEFT_DOOR;
}

bool RoomSystem::IsRightSideRoom(RoomId room) {
    return room == ROOM_EAST_HALL || room == ROOM_EAST_HALL_CORNER ||
           room == ROOM_RIGHT_DOOR;
}

i32 RoomSystem::GetRoomCount() {
    return s_roomCount - 1; // Exclude ROOM_NONE
}

RoomId RoomSystem::GetRoomByIndex(i32 index) {
    if (!s_initialized) Initialize();
    if (index < 0 || index >= s_roomCount - 1) return ROOM_NONE;
    return s_rooms[index].id;
}

} // namespace fnaf
