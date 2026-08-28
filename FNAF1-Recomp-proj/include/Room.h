/**
 * Five Nights at Freddy's 1 — Recompilation
 * Room.h: Room definitions and camera-to-room mapping
 */

#ifndef FNAF_ROOM_H
#define FNAF_ROOM_H

#include "Types.h"

namespace fnaf {

struct RoomInfo {
    RoomId     id;
    const char* name;
    CameraId   camera;
};

struct RoomConnection {
    RoomId from;
    RoomId to;
};

class RoomSystem {
public:
    static void Initialize();
    static const RoomInfo& GetRoomInfo(RoomId room);
    static CameraId GetRoomCamera(RoomId room);
    static RoomId GetCameraRoom(CameraId cam);
    static bool AreConnected(RoomId a, RoomId b);
    static i32 GetAdjacentRooms(RoomId room, RoomId* outRooms, i32 maxRooms);
    static bool IsLeftSideRoom(RoomId room);
    static bool IsRightSideRoom(RoomId room);
    static i32 GetRoomCount();
    static RoomId GetRoomByIndex(i32 index);
};

} // namespace fnaf

#endif // FNAF_ROOM_H
