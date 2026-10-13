#ifndef _WPAUDIO_TIME_H_
#define _WPAUDIO_TIME_H_

#include <wpaudio/altypes.h>
#include <SDL2/SDL.h>

// Time functions
static inline uint32 Time_GetTicks() {
    return SDL_GetTicks();
}

static inline uint64 Time_GetPerformanceCounter() {
    return SDL_GetPerformanceCounter();
}

static inline uint64 Time_GetPerformanceFrequency() {
    return SDL_GetPerformanceFrequency();
}

static inline void Time_Delay(uint32 ms) {
    SDL_Delay(ms);
}

// Implemented in AUD_Time.cpp
TimeStamp AudioGetTime(void);
void InitAudioTimer(void);

#endif
