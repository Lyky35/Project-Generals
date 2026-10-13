// GameSpy GP stub for MinGW cross-compilation
#ifndef __GP_H__
#define __GP_H__

typedef void* GPConnection;
typedef void* SBServer;
typedef struct { int dummy; } GPRecvBuddyRequestArg;
typedef struct { int dummy; } GPRecvBuddyMessageArg;
typedef struct { int dummy; } GPRecvBuddyStatusArg;
typedef struct { int dummy; } GPErrorArg;
typedef struct { int result; int profile; } GPConnectResponseArg;
typedef int GPProfile;
typedef int GPResult;
typedef int GPEnum;
typedef int GPErrorCode;
typedef unsigned int GPIBool;

#define GP_NONBLOCKING 1
#define GP_CONNECTION_TIMEDOUT 2
#define GP_NO_ERROR 0
#define GP_ONLINE 1
#define GP_OFFLINE 2
#define GP_RECV_GAME_INVITE 4
#define GP_PLAYING 8
#define GP_STAGING 16
#define GP_NICK_LEN 31
#define GP_UNIQUENICK_LEN 31
#define GP_EMAIL_LEN 100
#define GP_PASSWORD_LEN 31
#define GP_COUNTRYCODE_LEN 2
#define GP_REASON_LEN 512
#define GP_LOCATION_STRING_LEN 128
#define GP_STATUS_STRING_LEN 128
#define GP_FIRSTNAME_LEN 31
#define GP_LASTNAME_LEN 31
#define GP_HOMEPAGE_LEN 151
#define GP_GENERAL_ERROR 1

typedef struct {
    char email[33];
    char uniquenick[33];
    char nick[33];
    char firstname[33];
    char lastname[33];
    char homepage[256];
    int profileid;
} GPProfileInfo;

typedef struct {
    int profileid;
    char uniquenick[33];
    char nick[33];
    char email[33];
    unsigned int status;
    unsigned int ip;
    unsigned int port;
} GPIProfile;

#ifdef __cplusplus
extern "C" {
#endif

GPResult gpInitialize(GPConnection* connection, const char* cdkey, int version, const char* sdkVersion);
typedef void (*GPCallback)(...);
#define GP_ERROR 1
#define GP_RECV_BUDDY_REQUEST 1
#define GP_RECV_BUDDY_MESSAGE 2
#define GP_RECV_BUDDY_STATUS 3
#define GP_CONNECT 4
GPResult gpSetCallback(GPConnection* connection, int callbackType, GPCallback callback, void* userData);
void gpDestroy(GPConnection* connection);
GPResult gpProcessGPConnection(GPConnection* connection);
GPResult gpConnectNew(GPConnection* connection, const char* nick, const char* email,
    const char* password, int cdkey);
GPResult gpConnectPreauthenticated(GPConnection* connection, const char* loginTicket,
    const char* cdkey, int namespaceID);
GPResult gpDisconnect(GPConnection* connection);
GPResult gpProfileSearch(GPConnection* connection, const char* nick, const char* uniquenick,
    const char* email, int profileID);
GPResult gpProfileSearchByName(GPConnection* connection, const char* nick);
GPResult gpBuddyAdd(GPConnection* connection, const char* uniquenick);
GPResult gpBuddyRemove(GPConnection* connection, int profileid);
GPResult gpStatus(GPConnection* connection, GPEnum status, GPEnum away, const char* statusString);
GPResult gpSetStatus(GPConnection* connection, GPEnum status, GPEnum away, const char* statusString);
#define GP_CHATTING 1
GPResult gpGetUniqueNick(GPConnection* connection, const char* nick);
GPResult gpRegisterUniqueNick(GPConnection* connection, const char* uniquenick, const char* email);
GPResult gpDeleteProfile(GPConnection* connection, const char* password);

#ifdef __cplusplus
}
#endif

#endif // __GP_H__
