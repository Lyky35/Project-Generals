#ifndef _WPAUDIO_SOURCE_H_
#define _WPAUDIO_SOURCE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>

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
    ALuint size;  // Alias for Bytes
    ALuint Bytes;  // Alias for size
    ALboolean streaming;
    ALuint loopStart;
    ALuint loopEnd;
    ALboolean looped;
    char name[256];
    struct List Frames;  // List of frames for streaming
} AudioSample;

typedef struct AudioFormat {
    ALenum format;
    ALuint frequency;
    ALuint channels;
    ALuint bitsPerSample;
} AudioFormat;

// Frame structure for streaming
typedef struct AudioFrame {
    AudioSample *sample;
    void *Data;
    ALuint Bytes;
    ALuint sample_index;
    struct ListNode nd;
} AudioFrame;

// Function declarations
AudioSample* Source_Create(ALenum format, ALuint frequency, ALuint channels);
void Source_Destroy(AudioSample *src);

AudioFrame* AudioSampleFirstFrame(AudioSample *sample);
AudioFrame* AudioSampleNextFrame(AudioSample *sample, AudioFrame *frame);
AudioFrame* AudioSamplePrevFrame(AudioSample *sample, AudioFrame *frame);

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
        src->Bytes = 0;
        src->name[0] = 0;
        List_Init(&src->Frames);
    }
    return src;
}

static inline void Source_Destroy(AudioSample *src) {
    if (src) free(src);
}

static inline AudioFrame* AudioSampleFirstFrame(AudioSample *sample) {
    if (sample) {
        AudioFrame *frame = (AudioFrame*)malloc(sizeof(AudioFrame));
        if (frame) {
            frame->sample = sample;
            frame->Data = sample->data;
            frame->Bytes = sample->size;
            frame->sample_index = 0;
            return frame;
        }
    }
    return NULL;
}

static inline AudioFrame* AudioSampleNextFrame(AudioSample *sample, AudioFrame *frame) {
    (void)sample; (void)frame;
    return NULL;
}

static inline AudioFrame* AudioSamplePrevFrame(AudioSample *sample, AudioFrame *frame) {
    (void)sample; (void)frame;
    return NULL;
}

#endif // _WPAUDIO_SOURCE_H_
