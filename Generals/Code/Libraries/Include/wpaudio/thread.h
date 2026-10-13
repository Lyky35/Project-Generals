#ifndef _WPAUDIO_THREAD_H_
#define _WPAUDIO_THREAD_H_

#include <wpaudio/altypes.h>
#include <wpaudio/device.h>
#include <wpaudio/profiler.h>
#include <SDL2/SDL.h>

// Thread priority levels
typedef int AUD_ThreadPriority;
#define AUD_THREAD_PRI_NORMAL   0
#define AUD_THREAD_PRI_HIGH     1
#define AUD_THREAD_PRI_REALTIME 2

// Thread callback type
typedef int AUD_ThreadCB(void *thread, void *data);

// Thread structure (defined in AUD_Windows.cpp)
typedef struct _aud_thread AUD_Thread;

// Implemented in AUD_Windows.cpp
AUD_Thread *AUD_ThreadCreate(const char *name, AUD_ThreadPriority pri, AUD_ThreadCB *code);
void AUD_ThreadDestroy(AUD_Thread *thread);
void AUD_ThreadBeginCriticalSection(AUD_Thread *thread);
void AUD_ThreadEndCriticalSection(AUD_Thread *thread);
void AUD_ThreadSetData(AUD_Thread *thread, void *data);
void AUD_ThreadSetInterval(AUD_Thread *thread, TimeStamp interval);
TimeStamp AUD_ThreadGetInterval(AUD_Thread *thread);
char *AUD_ThreadName(AUD_Thread *thread);
ProfileCPU *AUD_ThreadCPUProfile(AUD_Thread *thread);
AudioServiceInfo *AUD_ThreadServiceInfo(AUD_Thread *thread);

#endif
