// GameSpy HTTP stub
#ifndef __GHTTP_H__
#define __GHTTP_H__

typedef void* GHTTPRequest;
typedef int GHTTPResult;
typedef int GHTTPBool;

#define GHTTPTrue 1
#define GHTTPFalse 0
#define GHTTPSuccess 0
#define GHTTPOutOfMemory 1
#define GHTTPBufferOverflow 2
#define GHTTPInvalidURL 3
#define GHTTPInvalidFile 4
#define GHTTPRedirectLimit 5
#define GHTTPInvalidFile 6

typedef void (*ghttpCompletedCallback)(GHTTPRequest request, GHTTPResult result, const char* buffer, int bufferLen, void* param);
typedef void (*ghttpProgressCallback)(GHTTPRequest request, GHTTPResult result, char* buffer, int bufferLen, int received, int total, void* param);

#ifdef __cplusplus
extern "C" {
#endif

GHTTPRequest ghttpGet(...);
GHTTPRequest ghttpGetHeaders(...);
GHTTPRequest ghttpHead(...);
GHTTPRequest ghttpGetEx(const char* URL, const char* headers, const char* buffer, int bufferLen, ghttpCompletedCallback completedCallback, ghttpProgressCallback progressCallback, void* param, GHTTPBool throttle);
GHTTPBool ghttpThink(void);
GHTTPBool ghttpCleanup(void);
GHTTPBool ghttpStartup(void);
GHTTPBool ghttpSetProxy(const char* server = 0, int port = 0);
GHTTPBool ghttpThink(void);
GHTTPBool ghttpSetRequestThrottle(GHTTPRequest request, int throttle);
const char* ghttpGetErrorString(GHTTPResult result);

#ifdef __cplusplus
}
#endif

#endif
