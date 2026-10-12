#ifndef _WPAUDIO_ATTRIBUTES_H_
#define _WPAUDIO_ATTRIBUTES_H_

#include <wpaudio/altypes.h>

// Audio attributes
typedef struct AudioAttribs {
    ALenum format;
    ALuint frequency;
    ALuint channels;
    ALuint bitsPerSample;
    ALuint size;
} AudioAttribs;

#endif
