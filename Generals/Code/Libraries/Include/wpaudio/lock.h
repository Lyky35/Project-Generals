#ifndef _WPAUDIO_LOCK_H_
#define _WPAUDIO_LOCK_H_

#include <SDL2/SDL.h>

// Thread lock
typedef SDL_mutex* Lock;

static inline Lock Lock_Create() {
    return SDL_CreateMutex();
}

static inline void Lock_Destroy(Lock lock) {
    if (lock) SDL_DestroyMutex(lock);
}

static inline void Lock_Enter(Lock lock) {
    if (lock) SDL_LockMutex(lock);
}

static inline void Lock_Leave(Lock lock) {
    if (lock) SDL_UnlockMutex(lock);
}

#endif
