#ifndef _WPAUDIO_DRIVER_H_
#define _WPAUDIO_DRIVER_H_

#include <wpaudio/altypes.h>

// Audio driver
typedef struct {
    ALboolean initialized;
    ALuint maxChannels;
} AudioDriver;

static inline AudioDriver* Driver_Create() {
    AudioDriver *drv = (AudioDriver*)malloc(sizeof(AudioDriver));
    if (drv) {
        drv->initialized = AL_TRUE;
        drv->maxChannels = 32;
    }
    return drv;
}

static inline void Driver_Destroy(AudioDriver *drv) {
    if (drv) free(drv);
}

#endif
