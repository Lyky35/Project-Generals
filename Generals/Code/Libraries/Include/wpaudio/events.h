#ifndef _WPAUDIO_EVENTS_H_
#define _WPAUDIO_EVENTS_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/level.h>
#include <wpaudio/cache.h>
#include <wpaudio/attributes.h>
#include <wpaudio/channel.h>
#include <wpaudio/time.h>

// Int type (from BaseType.h)
typedef int Int;

// Priority levels for events
typedef int AudioPriority;
#define AUDIO_NUM_EVENT_PRIORITIES 8

// Event control flags
typedef int AudioEventControl;
#define AUDIO_EVENT_CTRL_ATTACK     0x0001
#define AUDIO_EVENT_CTRL_DECAY      0x0002
#define AUDIO_EVENT_CTRL_LOOP       0x0004
#define AUDIO_EVENT_CTRL_RANDOM     0x0008
#define AUDIO_EVENT_CTRL_ALL        0x0010
#define AUDIO_EVENT_CTRL_INTERRUPT  0x0020
#define AUDIO_EVENT_CTRL_POSTDELAY  0x0040
#define AUDIO_EVENT_CTRL_AMBIENT    0x0080

// Loop flag for channel control
#define mAUDIO_LOOP 0x0100

// Milliseconds macro (values are already in ms)
#define MSECONDS(x) (x)

// Volume limits
#ifndef AUDIO_VOLUME_MIN
#define AUDIO_VOLUME_MIN 0
#endif

// Event limits
#define MAX_AUDIO_EVENT_SAMPLES 8

// Forward declarations
struct AudioEventClassTag;
struct AudioEventTag;

typedef struct AudioEventClassTag AudioEventClass;
typedef struct AudioEventTag AudioEvent;

// Audio event handle - reference to a playing event
struct AudioEventHandleTag {
    AudioEvent *event;
    int stamp;
    AudioEventClass *eclass;
    int id;
};
typedef struct AudioEventHandleTag AudioEventHandle;

// Invalid stamp
#define AUDIO_INVALID_STAMP 0

// Simple event type enum (used by some callers)
typedef enum {
    AUDIO_EVENT_PLAY = 0,
    AUDIO_EVENT_STOP,
    AUDIO_EVENT_PAUSE,
    AUDIO_EVENT_RESUME,
    AUDIO_EVENT_VOLUME_CHANGE,
    AUDIO_EVENT_PAN_CHANGE,
    AUDIO_EVENT_FREQUENCY_CHANGE
} AudioEventType;

// Implemented in AUD_Events.cpp
void AudioEventClassReset(AudioEventClass *eclass);
void AudioEventKill(AudioEvent *event);
void AudioEventPause(AudioEvent *event);
void AudioEventResume(AudioEvent *event);
int AudioEventIsPaused(AudioEvent *event);
void AudioEventService(AudioEvent *event);
int AudioEventNeverEnds(AudioEvent *event);
void AudioKillAllEvents(void);
void AudioFlushAllDeadEvents(void);
AudioChannel *AudioDeviceGetChannel(AudioDevice *dev, AudioChannelType type);

// AudioEventHandle functions
void AudioEventHandleInit(AudioEventHandle *handle);
void AudioEventHandleStop(AudioEventHandle *handle);
void AudioEventHandleStopLooping(AudioEventHandle *handle);
AudioEvent *AudioEventHandleGet(AudioEventHandle *handle);
AudioEventClass *AudioEventHandleGetClass(AudioEventHandle *handle);

// Channel attribute setters (implemented in AUD_Channel.cpp or similar)
void AudioChannelSetPitch(AudioChannel *chan, int pitch);
void AudioChannelSetVolume(AudioChannel *chan, int volume);

// Random number picker (utility)
static inline int AudioRandomPick(int lo, int hi) {
    if (hi <= lo) return lo;
    return lo + (rand() % (hi - lo + 1));
}

#endif // _WPAUDIO_EVENTS_H_
