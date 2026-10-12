#ifndef _WPAUDIO_LEVEL_H_
#define _WPAUDIO_LEVEL_H_

#include <wpaudio/altypes.h>

// Audio level/mixing
typedef struct {
    float masterVolume;
    float musicVolume;
    float sfxVolume;
    float voiceVolume;
} AudioLevels;

static inline AudioLevels* Levels_Create() {
    AudioLevels *lvl = (AudioLevels*)malloc(sizeof(AudioLevels));
    if (lvl) {
        lvl->masterVolume = 1.0f;
        lvl->musicVolume = 0.8f;
        lvl->sfxVolume = 1.0f;
        lvl->voiceVolume = 1.0f;
    }
    return lvl;
}

static inline void Levels_Destroy(AudioLevels *lvl) {
    if (lvl) free(lvl);
}

#endif
