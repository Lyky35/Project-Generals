// GameSpy PEER stub for MinGW cross-compilation
#ifndef __PEER_H__
#define __PEER_H__

typedef void* PEER;
typedef int PEERJoinResult;
typedef int PEERInUse;
#define PEER_IN_USE 1
#define PEER_NO_ERROR 0
typedef int PEERMessageType;
typedef int GServer;
typedef void (*peerEnumPlayersCallback)(...);
typedef void (*peerListingGamesCallback)(...);
typedef int RoomType;
typedef int StagingRoom;
typedef int GroupRoom;
#define PEER_FLAG_OP 0x00000001
#define PEER_FLAG_VOICE 0x00000002
#define PEER_FLAG_BROADCASTING 0x00000004

// PEER error codes
#define PEERJoinSuccess 0
#define PEERFullRoom 1
#define PEERInviteOnlyRoom 2
#define PEERBannedFromRoom 3
#define PEERBadPassword 4
#define PEERAlreadyInRoom 5
#define PEERNoConnection 6

// PEER message types
#define PEER_CLEAR 0
#define PEER_ADD 1
#define PEER_UPDATE 2
#define PEER_REMOVE 3
#define ROOM_TYPE_GAME 0
#define ROOM_TYPE_CHAT 1
#define ROOM_TYPE_TITLE 2
typedef int PEERBool;
typedef unsigned int PEERConnectionType;
typedef unsigned int PEERError;

#define PI_ROOMS 1
#define PI_BROADCASTS 2
#define PI_MESSAGES 4
#define PI_UNIQUENICKS 8

#define PEERFalse 0
#define PEERTrue 1

typedef enum { PEERStop, PEERStarting, PEERStaying, PEERRunning } PEERStatus;

typedef enum {
    PEERErrorNoError,
    PEERErrorGeneral,
    PEERErrorOutOfMemory,
    PEERErrorNotInitialized,
    PEERErrorSocketFailed,
    PEERErrorAlreadyConnected,
    PEERErrorNotConnected,
    PEERErrorRoomInvalidName,
    PEERErrorRoomFull,
    PEERErrorRoomAlreadyJoined,
    PEERErrorRoomNotJoined,
    PEERErrorNickInvalid,
    PEERErrorNickInUse,
    PEERErrorNickUnavailable,
    PEERErrorConnectionFailed,
    PEERErrorBadServer,
    PEERErrorQueryFailed,
    PEERErrorDuplicateNick,
    PEERErrorNoSearch,
    PEERErrorNoRoomList,
    PEERErrorTooManyRooms,
    PEERErrorRoomListRequest,
    PEERErrorBadServerList,
    PEERErrorConnectionLost,
    PEERErrorSystemError,
    PEERErrorMagic,
    PEERErrorLoginTicket,
    PEERErrorBadPrep,
    PEERErrorProtocolViolation,
    PEERErrorUnexpectedLogout,
    PEERErrorBan,
    PEERErrorPeerMetadata,
    PEERErrorNotFound,
    PEERErrorTitleNotFound,
    PEERErrorInvalidSubtopic,
    PEERErrorLoginDenied,
    PEERErrorInvalidData,
    PEERErrorCategory,
    PEERErrorUniqueNickError,
    PEERErrorLogonDenied,
    PEERErrorUnspecified
} PEERErrorType;

#ifdef __cplusplus
extern "C" {
#endif

PEER peerStartup(void* params, const char* ticket, const char* game,
    const char* gamename, unsigned int port, int version);
PEERBool peerShutdown(PEER peer);
PEERStatus peerGetConnectionStatus(PEER peer, PEERError* error);
PEERBool peerThink(PEER peer);
PEERBool peerConnect(PEER peer, const char* nick, void* callback, void* userData, int blocking);
PEERBool peerDisconnect(PEER peer);
PEERBool peerJoinRoom(PEER peer, const char* room, PEERConnectionType type);
PEERBool peerLeaveRoom(PEER peer, PEERConnectionType type);
PEERBool peerJoinUniqueRoom(PEER peer, const char* uniquenick, PEERConnectionType type);
PEERBool peerTitleLogin(PEER peer, const char* uniquenick, const char* email);
const char* peerGetNick(PEER peer);
const char* peerGetLocalIP(PEER peer);
PEERBool peerRetryWithNick(PEER peer, const char* nick);
PEERBool peerListGroupRooms(PEER peer, void* callback, void* userData);
PEERBool peerSetReady(PEER peer, PEERBool ready);
PEERBool peerSetUniqueNick(PEER peer, const char* uniquenick);
PEERBool peerSetTitle(PEER peer, const char* title);
PEERBool peerBroadcastMessage(PEER peer, const char* message);
PEERBool peerMessageRoom(PEER peer, PEERConnectionType type, const char* message);
PEERBool peerAwayMessage(PEER peer, PEERConnectionType type, const char* message);
PEERBool peerGetRoomList(PEER peer);
PEERBool peerStopRoomList(PEER peer);
PEERBool peerStopGetRoomList(PEER peer);
PEERBool peerEnumRooms(PEER peer, void* callback, void* instance);
PEERBool peerEnumPlayers(PEER peer, PEERConnectionType type, void* callback, void* instance);
PEERBool peerEnumBuddy(PEER peer, void* callback, void* instance);
PEERBool peerAddBuddy(PEER peer, const char* nick);
PEERBool peerRemoveBuddy(PEER peer, const char* nick);
PEERBool peerSetReady(PEER peer, PEERBool ready);
PEERBool peerGetReady(PEER peer);

#ifdef __cplusplus
}
#endif

#endif // __PEER_H__
