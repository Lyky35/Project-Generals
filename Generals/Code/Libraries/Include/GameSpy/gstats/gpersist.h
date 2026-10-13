// GameSpy gpersist stub
#ifndef __GPERSIST_H__
#define __GPERSIST_H__

typedef void*PersistConnection;
typedef int GPersistResult;
typedef int GPersistErrorCode;

#define GPERSIST_NO_ERROR 0
#define GPERSIST_GENERAL_ERROR 1

#ifdef __cplusplus
extern "C" {
#endif

GPersistResult gpersistStartup(PersistConnection* connection);
void gpersistDestroy(PersistConnection* connection);
GPersistResult gpersistThink(PersistConnection* connection);
GPersistResult gpersistLogin(PersistConnection* connection, const char* nick, const char* uniquenick);
GPersistResult gpersistGetGlobalInt(PersistConnection* connection, const char* key, int* value);
GPersistResult gpersistSetGlobalInt(PersistConnection* connection, const char* key, int value);
GPersistResult gpersistGetGlobalString(PersistConnection* connection, const char* key, char* value, int len);
GPersistResult gpersistSetGlobalString(PersistConnection* connection, const char* key, const char* value);
GPersistResult gpersistGetProfileInt(PersistConnection* connection, const char* key, int* value);
GPersistResult gpersistSetProfileInt(PersistConnection* connection, const char* key, int value);

#ifdef __cplusplus
}
#endif

#endif
