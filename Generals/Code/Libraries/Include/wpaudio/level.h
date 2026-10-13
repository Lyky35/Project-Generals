#ifndef _WPAUDIO_LEVEL_H_
#define _WPAUDIO_LEVEL_H_

#include <wpaudio/altypes.h>

// Audio level - smooth volume/pan/pitch transitions
typedef struct AudioLevel {
    int level;        // current level (scaled)
    int newLevel;     // target level (scaled)
    int change;       // change per tick (scaled)
    int duration;     // duration in ms
    int flags;        // state flags
    TimeStamp lastTime;
} AudioLevel;

// Level constants
#define AUDIO_VOLUME_MAX 1000
#define AUDIO_PAN_CENTER 0
#define AUDIO_LEVEL_MAX  1000
#define AUDIO_LEVEL_MIN  0
#define AUDIO_LEVEL_MAX_VAL 1000
#define AUDIO_LEVEL_MIN_VAL (-1000)

// Fixed-point scale (8-bit fraction)
#define AUDIO_LEVEL_SCALE 8

// Level state flags
#define AUDIO_LEVEL_SET     0x0001
#define AUDIO_LEVEL_CHANGED 0x0002
#define AUDIO_LEVEL_ACTIVE  0x0004

// Apply a delta to a level value
#define AUDIO_LEVEL_APPLY(level, val) ((level)->level + (val))

#define SECONDS(x) ((x) * 1000)

// Implemented in AUD_Level.cpp
void AudioLevelInit(AudioLevel *level, int startLevel);
void AudioLevelSet(AudioLevel *level, int newLevel);
void AudioLevelForce(AudioLevel *level);
int AudioLevelApply(AudioLevel *level, int val);
void AudioLevelAdjust(AudioLevel *level, int newLevel);
void AudioLevelSetDuration(AudioLevel *level, TimeStamp time, int range);
int AudioLevelUpdate(AudioLevel *level);
int AudioLevelGetVal(AudioLevel *level);

// Inline implementations
static inline int AudioLevelChanged(AudioLevel *level)
{
    if (!level) return 0;
    if (level->flags & AUDIO_LEVEL_CHANGED) {
        level->flags &= ~AUDIO_LEVEL_CHANGED;
        return 1;
    }
    return 0;
}

static inline void AudioLevelUsed(AudioLevel *level)
{
    if (level) level->flags &= ~AUDIO_LEVEL_CHANGED;
}

static inline int AudioLevelGet(AudioLevel *level)
{
    if (!level) return 0;
    return level->level >> AUDIO_LEVEL_SCALE;
}

#endif
