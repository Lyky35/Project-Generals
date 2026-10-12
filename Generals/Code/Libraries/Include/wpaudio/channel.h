#ifndef _WPAUDIO_CHANNEL_H_
#define _WPAUDIO_CHANNEL_H_

#include <wpaudio/altypes.h>
#include <wpaudio/source.h>

// Audio channel
typedef struct AudioChannel {
    ALuint source;
    ALuint buffer;
    ALuint state;
    float volume;
    float pan;
    ALuint frequency;
    ALuint flags;
    struct AudioSample *sample;
    struct AudioControl *control;
} AudioChannel;

typedef struct AudioControl {
    ALuint id;
    ALuint channel;
    ALuint type;
    float value;
} AudioControl;

static inline AudioChannel* Channel_Create() {
    AudioChannel *ch = (AudioChannel*)malloc(sizeof(AudioChannel));
    if (ch) {
        ch->source = 0;
        ch->buffer = 0;
        ch->state = 0;
        ch->volume = 1.0f;
        ch->pan = 0.0f;
        ch->frequency = 44100;
        ch->flags = 0;
        ch->sample = NULL;
        ch->control = NULL;
    }
    return ch;
}

static inline void Channel_Destroy(AudioChannel *ch) {
    if (ch) free(ch);
}

static inline void Channel_Play(AudioChannel *ch, ALuint buffer) {
    if (ch) {
        ch->buffer = buffer;
        ch->state = 1;
    }
}

static inline void Channel_Stop(AudioChannel *ch) {
    if (ch) ch->state = 0;
}

static inline void Channel_SetVolume(AudioChannel *ch, float vol) {
    if (ch) ch->volume = vol;
}

static inline void Channel_SetPan(AudioChannel *ch, float pan) {
    if (ch) ch->pan = pan;
}

static inline void Channel_SetFrequency(AudioChannel *ch, ALuint freq) {
    if (ch) ch->frequency = freq;
}

#endif // _WPAUDIO_CHANNEL_H_
