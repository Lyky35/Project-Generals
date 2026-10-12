#ifndef _WPAUDIO_ATTRIBUTES_H_
#define _WPAUDIO_ATTRIBUTES_H_

#include <wpaudio/altypes.h>
#include <wpaudio/level.h>

// Audio attributes
typedef struct AudioAttribs {
    ALenum format;
    ALuint frequency;
    ALuint channels;
    ALuint bitsPerSample;
    ALuint size;
    int VolumeLevel;
    int PitchLevel;
    int PanPosition;
    int PitchDuration;
    int VolumeDuration;
    int PanDuration;
} AudioAttribs;

// Audio level structure
typedef struct AudioLevel {
    int current;
    int target;
    int duration;
    int elapsed;
    int min;
    int max;
} AudioLevel;

// Function declarations
void AudioAttribsInit(AudioAttribs *attr);
void AudioAttribsUpdate(AudioAttribs *attr);
int AudioAttribsChanged(AudioAttribs *attr);
void AudioAttribsApply(AudioAttribs *attr, AudioAttribs *mod);
void AudioAttribsUsed(AudioAttribs *attr);
int AudioAttribsCalcPitch(AudioAttribs *attr, int pitch);
void AudioAttribsSetPitchDuration(AudioAttribs *attr, int duration, int scale);
void AudioAttribsSetVolumeDuration(AudioAttribs *attr, int duration, int scale);
void AudioAttribsSetPanDuration(AudioAttribs *attr, int duration, int scale);
int AudioAttribsGetPitch(AudioAttribs *attr);
int AudioAttribsGetVolume(AudioAttribs *attr);
int AudioAttribsGetPan(AudioAttribs *attr);
void AudioAttribsSetPitch(AudioAttribs *attr, int pitch);
void AudioAttribsSetVolume(AudioAttribs *attr, int volume);
void AudioAttribsSetPan(AudioAttribs *attr, int pan);

void AudioLevelInit(int *level, int max);
int AudioLevelChanged(int *level);
void AudioLevelUpdate(int *level);
void AudioLevelSet(int *level, int value);
int AudioLevelApply(int *level, int value);
int AudioLevelGet(int *level);

int AudioAttribsGetPitch(AudioAttribs *attr);
int AudioAttribsGetVolume(AudioAttribs *attr);
int AudioAttribsGetPan(AudioAttribs *attr);
void AudioAttribsSetPitch(AudioAttribs *attr, int pitch);
void AudioAttribsSetVolume(AudioAttribs *attr, int volume);
void AudioAttribsSetPan(AudioAttribs *attr, int pan);
void AudioAttribsSetPitchDuration(AudioAttribs *attr, int duration, int scale);
void AudioAttribsSetVolumeDuration(AudioAttribs *attr, int duration, int scale);
void AudioAttribsSetPanDuration(AudioAttribs *attr, int duration, int scale);

void AudioAttribsApply(AudioAttribs *attr, AudioAttribs *mod);
void AudioAttribsUsed(AudioAttribs *attr);
int AudioAttribsCalcPitch(AudioAttribs *attr, int pitch);

void AudioLevelInit(int *level, int max);
int AudioLevelChanged(int *level);
void AudioLevelUpdate(int *level);
void AudioLevelSet(int *level, int value);
int AudioLevelApply(int *level, int value);
int AudioLevelGet(int *level);

void AudioLevelChanged(AudioAttribs *attr);
void AudioLevelUpdate(AudioAttribs *attr);

#define SECONDS(x) ((x) * 1000)

#endif
