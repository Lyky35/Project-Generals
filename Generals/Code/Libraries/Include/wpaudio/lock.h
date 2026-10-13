// wpaudio/lock.h - Thread synchronization lock
// Original Westwood Studios implementation

#ifndef _WPAUDIO_LOCK_H_
#define _WPAUDIO_LOCK_H_

#include <wpaudio/altypes.h>
#include <SDL2/SDL.h>

// Lock - simple mutex wrapper
typedef struct {
    SDL_mutex *mutex;
    int locked;
} Lock;

// Implemented in AUD_Lock.cpp
void LockInit(volatile Lock *lock);
void LockAcquire(volatile Lock *lock);
void LockRelease(volatile Lock *lock);
int Locked(volatile Lock *lock);

// Inline helper - returns TRUE if the lock is NOT held
static inline int NotLocked(volatile Lock *lock)
{
    return lock && !lock->locked;
}

#endif // _WPAUDIO_LOCK_H_
