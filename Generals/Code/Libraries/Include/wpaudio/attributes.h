// wpaudio/attributes.h - Audio attribute management
// Original Westwood Studios implementation

#ifndef _WPAUDIO_ATTRIBUTES_H_
#define _WPAUDIO_ATTRIBUTES_H_

#include <wpaudio/altypes.h>
#include <wpaudio/level.h>

// Audio level constants
#define AUDIO_LEVEL_MAX 1000
#define AUDIO_PAN_LEFT  (-AUDIO_LEVEL_MAX)
#define AUDIO_PAN_RIGHT (AUDIO_LEVEL_MAX)

// Audio attributes - volume, pitch, pan with duration support
typedef struct AudioAttribs {
    AudioLevel VolumeLevel;
    AudioLevel PitchLevel;
    AudioLevel PanPosition;
    int PitchDuration;
    int VolumeDuration;
    int PanDuration;
} AudioAttribs;

// Implemented in AUD_Attributes.cpp
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

// Adjustments
void AudioAttribsAdjustVolume(AudioAttribs *attr, int amount);
int AudioAttribsVolumeAdjusted(AudioAttribs *attr);

#endif // _WPAUDIO_ATTRIBUTES_H_
