#ifndef _WPAUDIO_TIME_H_
#define _WPAUDIO_TIME_H_

#include <SDL2/SDL.h>

// Time functions
static inline ALuint Time_GetTicks() {
    return SDL_GetTicks();
}

static inline ALuint Time_GetPerformanceCounter() {
    return (ALuint)SDL_GetPerformanceCounter();
}

static inline ALuint Time_GetPerformanceFrequency() {
    return (ALuint)SDL_GetPerformanceFrequency();
}

static inline void Time_Delay(ALuint ms) {
    SDL_Delay(ms);
}

#endif
