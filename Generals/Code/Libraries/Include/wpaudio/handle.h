#ifndef _WPAUDIO_HANDLE_H_
#define _WPAUDIO_HANDLE_H_

#include <wpaudio/altypes.h>

// Handle types
typedef ALuint HSOURCE;
typedef ALuint HBUFFER;
typedef ALuint HCHANNEL;

#define INVALID_HANDLE 0

static inline ALboolean Handle_IsValid(ALuint handle) {
    return handle != INVALID_HANDLE;
}

#endif
