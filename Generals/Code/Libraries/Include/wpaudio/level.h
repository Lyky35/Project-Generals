#ifndef _WPAUDIO_LEVEL_H_
#define _WPAUDIO_LEVEL_H_

#include <wpaudio/altypes.h>

// Audio level structure
typedef struct {
    int current;
    int target;
    int duration;
    int elapsed;
    int min;
    int max;
} AudioLevel;

// Audio level constants
#define AUDIO_VOLUME_MAX 1000
#define AUDIO_PAN_CENTER 0

// Function declarations
void AudioLevelInit(int *level, int max);
int AudioLevelChanged(int *level);
void AudioLevelUpdate(int *level);
void AudioLevelSet(int *level, int value);
int AudioLevelApply(int *level, int value);
int AudioLevelGet(int *level);

#define SECONDS(x) ((x) * 1000)

#endif
