#ifndef _WPAUDIO_CACHE_H_
#define _WPAUDIO_CACHE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>

// Audio cache
typedef struct {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
} AudioCache;

static inline AudioCache* Cache_Create(ALuint maxSize) {
    AudioCache *cache = (AudioCache*)malloc(sizeof(AudioCache));
    if (cache) {
        cache->entries = (List*)malloc(sizeof(List));
        List_Init(cache->entries);
        cache->maxSize = maxSize;
        cache->currentSize = 0;
    }
    return cache;
}

static inline void Cache_Destroy(AudioCache *cache) {
    if (cache) {
        List_Clear(cache->entries);
        free(cache->entries);
        free(cache);
    }
}

#endif
