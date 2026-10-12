#ifndef _WPAUDIO_DEVICE_H_
#define _WPAUDIO_DEVICE_H_

#include <wpaudio/altypes.h>

// Audio device
typedef struct {
    ALCdevice *device;
    ALCcontext *context;
    ALboolean initialized;
} AudioDevice;

static inline AudioDevice* Device_Create() {
    AudioDevice *dev = (AudioDevice*)malloc(sizeof(AudioDevice));
    if (dev) {
        dev->device = alcOpenDevice(NULL);
        dev->context = dev->device ? alcCreateContext(dev->device, NULL) : NULL;
        dev->initialized = (dev->context != NULL);
    }
    return dev;
}

static inline void Device_Destroy(AudioDevice *dev) {
    if (dev) {
        if (dev->context) alcDestroyContext(dev->context);
        if (dev->device) alcCloseDevice(dev->device);
        free(dev);
    }
}

static inline ALboolean Device_MakeCurrent(AudioDevice *dev) {
    return dev && dev->context ? alcMakeContextCurrent(dev->context) : AL_FALSE;
}

#endif
