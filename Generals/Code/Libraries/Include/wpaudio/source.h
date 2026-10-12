#ifndef _WPAUDIO_SOURCE_H_
#define _WPAUDIO_SOURCE_H_

#include <wpaudio/altypes.h>

// Audio sample
typedef struct AudioSample {
    ALuint id;
    ALuint buffer;
    ALenum format;
    ALuint frequency;
    ALuint channels;
    ALuint size;
    ALuint length;
    ALboolean streaming;
    ALuint loopStart;
    ALuint loopEnd;
    ALboolean looped;
    void *data;
} AudioSample;

typedef struct AudioFormat {
    ALenum format;
    ALuint frequency;
    ALuint channels;
    ALuint bitsPerSample;
} AudioFormat;

static inline AudioSample* Source_Create(ALenum format, ALuint frequency, ALuint channels) {
    AudioSample *src = (AudioSample*)malloc(sizeof(AudioSample));
    if (src) {
        src->id = 0;
        src->buffer = 0;
        src->format = format;
        src->frequency = frequency;
        src->channels = channels;
        src->size = 0;
        src->length = 0;
        src->streaming = AL_FALSE;
        src->loopStart = 0;
        src->loopEnd = 0;
        src->looped = AL_FALSE;
        src->data = NULL;
    }
    return src;
}

static inline void Source_Destroy(AudioSample *src) {
    if (src) free(src);
}

#endif // _WPAUDIO_SOURCE_H_
