#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include <atomic>

namespace GameSpy {

using PEER = void*;
using GPConnection = void*;
using GServer = void*;
using GPProfile = int;
using PEERBool = int;
using RoomType = int;
using MessageType = int;
using GHTTPRequest = int;
using GHTTPState = int;

constexpr int PEERTrue = 1;
constexpr int PEERFalse = 0;
constexpr int GHTTPFalse = 0;
constexpr int GHTTPTrue = 1;
constexpr int PEER_STOP_REPORTING = 1;

enum class PEERJoinResult {
    PEER_JOIN_SUCCESS,
    PEER_JOIN_ERROR,
    PEER_JOIN_FULL
};

struct PEERCallbacks {
    void (*connected)(PEER, void*) = nullptr;
    void (*disconnected)(PEER, void*) = nullptr;
    void (*message)(PEER, const char*, MessageType, void*) = nullptr;
    void (*playerMessage)(PEER, const char*, const char*, MessageType, void*) = nullptr;
    void (*playerJoined)(PEER, const char*, RoomType, void*) = nullptr;
    void (*playerLeft)(PEER, const char*, RoomType, void*) = nullptr;
    void (*roomList)(PEER, const char*, void*) = nullptr;
    void (*roomMessage)(PEER, RoomType, const char*, const char*, MessageType, void*) = nullptr;
    void (*keyList)(PEER, RoomType, const char*, const char*, int, char**, char**, void*) = nullptr;
    void (*getRoomKey)(PEER, RoomType, const char*, const char*, void*) = nullptr;
    void (*setRoomKey)(PEER, RoomType, const char*, const char*, const char*, void*) = nullptr;
    void (*playerKey)(PEER, const char*, RoomType, const char*, void*) = nullptr;
    void (*stateChange)(PEER, void*) = nullptr;
};

struct GPConnectResponseArg {
    int errorCode = 0;
    const char* errorMessage = nullptr;
    GPProfile profile = 0;
};

struct GPErrorArg {
    int errorCode = 0;
    const char* errorMessage = nullptr;
};

struct GPRecvBuddyRequestArg {
    GPProfile from;
    const char* message;
};

struct GPRecvBuddyMessageArg {
    GPProfile from;
    const char* message;
};

struct GPRecvBuddyStatusArg {
    GPProfile profile;
    int status;
    const char* statusString;
};

struct GServer {
    const char* ip = nullptr;
    int port = 0;
    const char* hostname = nullptr;
    const char* mapname = nullptr;
    int numPlayers = 0;
    int maxPlayers = 0;
    const char* gameType = nullptr;
    int ping = 0;
};

inline PEER peerInitialize(PEERCallbacks*) { return nullptr; }
inline void peerSetTitle(PEER, const char*, const char*, const char*) {}
inline PEERBool peerConnect(PEER, const char*, int, void (*)(PEER, PEERJoinResult, void*), void*, PEERBool) { return PEERFalse; }
inline void peerDisconnect(PEER) {}
inline void peerShutdown(PEER) {}
inline void peerThink(PEER) {}
inline PEERBool peerIsConnected(PEER) { return PEERFalse; }
inline void peerJoinGroupRoom(PEER, int, void (*)(PEER, PEERJoinResult, const char*, void*), void*, PEERBool) {}
inline void peerJoinStagingRoom(PEER, GServer, const char*, void (*)(PEER, PEERJoinResult, const char*, void*), void*, PEERBool) {}
inline void peerCreateStagingRoom(PEER, const char*, int, const char*, void (*)(PEER, PEERJoinResult, const char*, void*), void*, PEERBool) {}
inline void peerCreateStagingRoomWithSocket(PEER, const char*, int, const char*, int, void (*)(PEER, PEERJoinResult, const char*, void*), void*, PEERBool) {}
inline void peerLeaveRoom(PEER, RoomType, const char*) {}
inline void peerListGroupRooms(PEER, void (*)(PEER, const char*, void*), void*, PEERBool) {}
inline void peerMessageRoom(PEER, RoomType, const char*, MessageType) {}
inline void peerMessagePlayer(PEER, const char*, const char*, MessageType) {}
inline void peerUTMRoom(PEER, RoomType, const char*, const char*, PEERBool) {}
inline void peerUTMPlayer(PEER, const char*, const char*, const char*, PEERBool) {}
inline void peerEnumPlayers(PEER, RoomType, void (*)(PEER, const char*, int, void*), void*) {}
inline int peerGetPlayerFlags(PEER, const char*, RoomType) { return 0; }
inline void peerGetPlayerInfoNoWait(PEER, const char*, unsigned*, int*) {}
inline void peerGetPlayerProfileID(PEER, const char*, void (*)(PEER, int, GPProfile, void*), void*, PEERBool) {}
inline void peerSetReady(PEER, PEERBool) {}
inline void peerSetRoomKeys(PEER, RoomType, const char*, int, const char**, const char**) {}
inline void peerGetRoomKeys(PEER, RoomType, const char*, int, const char**, void (*)(PEER, const char*, int, char**, char**, void*), void*, PEERBool) {}
inline void peerSetRoomWatchKeys(PEER, RoomType, int, const char**) {}
inline void peerSetGlobalKeys(PEER, int, const char**, const char**) {}
inline void peerSetGlobalWatchKeys(PEER, RoomType, int, const char**) {}
inline void peerStartGame(PEER, const char*, int) {}
inline void peerStopGame(PEER) {}
inline void peerUpdateGame(PEER, GServer, PEERBool) {}
inline void peerStartListingGames(PEER, const char**, int, const char*, void (*)(PEER, int, const char*, GServer*, int, void*), void*) {}
inline void peerStopListingGames(PEER) {}
inline void peerAuthenticateCDKey(PEER, const char*, void (*)(PEER, int, const char*, void*), void*, PEERBool) {}
inline void peerStateChanged(PEER) {}
inline void peerRetryWithNick(PEER, const char*) {}
inline const char* peerGetLocalIP(PEER) { return "127.0.0.1"; }
inline int peerParseQuery(PEER, const char*, int*, void*) { return 0; }

inline GPConnection gpInitialize() { return nullptr; }
inline void gpDestroy(GPConnection) {}
inline void gpConnect(GPConnection, GPProfile, const char*, void (*)(GPConnection, GPConnectResponseArg*, void*), void*, int) {}
inline void gpProcess(GPConnection) {}
inline int gpIsConnected(GPConnection) { return 0; }
inline int gpGetProfile(GPConnection) { return 0; }

inline GHTTPState ghttpGet(const char*, GHTTPFalse, void (*)(GHTTPRequest, int, const char*, void*), void*) { return 0; }
inline GHTTPState ghttpHead(const char*, GHTTPFalse, void (*)(GHTTPRequest, int, const char*, void*), void*) { return 0; }
inline void ghttpThink() {}
inline const char* ghttpGetHeaders(GHTTPRequest) { return ""; }
inline void ghttpSetProxy(const char*) {}

inline int IsStatsConnected() { return 0; }
inline void GetPersistDataValues(int, GPProfile, int, int, const char*, void (*)(int, int, int, void*), void*) {}
inline void SetPersistDataValues(int, GPProfile, int, int, const char*, void (*)(int, int, int, void*), void*) {}
inline void PreAuthenticatePlayerPM(int, GPProfile, int, void (*)(int, int, void*), void*) {}

inline const char* SBServerGetStringValue(GServer, const char*, const char*) { return ""; }
inline const char* SBServerGetPrivateInetAddress(GServer) { return ""; }
inline int SBServerGetPrivateQueryPort(GServer) { return 0; }
inline const char* SBServerGetPublicInetAddress(GServer) { return ""; }

} // namespace GameSpy
