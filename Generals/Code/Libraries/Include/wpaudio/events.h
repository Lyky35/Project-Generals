#ifndef _WPAUDIO_EVENTS_H_
#define _WPAUDIO_EVENTS_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>

// Audio events
typedef enum {
    AUDIO_EVENT_PLAY = 0,
    AUDIO_EVENT_STOP,
    AUDIO_EVENT_PAUSE,
    AUDIO_EVENT_RESUME,
    AUDIO_EVENT_VOLUME_CHANGE,
    AUDIO_EVENT_PAN_CHANGE,
    AUDIO_EVENT_FREQUENCY_CHANGE
} AudioEventType;

typedef struct {
    AudioEventType type;
    void *data;
    ALuint channel;
} AudioEvent;

static inline AudioEvent* Event_Create(AudioEventType type, void *data, ALuint channel) {
    AudioEvent *evt = (AudioEvent*)malloc(sizeof(AudioEvent));
    if (evt) {
        evt->type = type;
        evt->data = data;
        evt->channel = channel;
    }
    return evt;
}

static inline void Event_Destroy(AudioEvent *evt) {
    if (evt) free(evt);
}

#endif
