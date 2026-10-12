#ifndef _WPAUDIO_THREAD_H_
#define _WPAUDIO_THREAD_H_

#include <SDL2/SDL.h>

// Thread wrapper
typedef SDL_Thread Thread;

static inline Thread* Thread_Create(int (*fn)(void*), void *data, const char *name) {
    return SDL_CreateThread(fn, name, NULL);
}

static inline void Thread_Wait(Thread *thread, int *status) {
    if (thread) SDL_WaitThread(thread, status);
}

static inline void Thread_Detach(Thread *thread) {
    if (thread) SDL_DetachThread(thread);
}

#endif
