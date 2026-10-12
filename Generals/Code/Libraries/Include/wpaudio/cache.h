#ifndef _WPAUDIO_CACHE_H_
#define _WPAUDIO_CACHE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/source.h>

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
} AudioCacheItem;

// Audio cache
typedef struct {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
} AudioCache;

// Function declarations
AudioCache* Cache_Create(ALuint maxSize);
void Cache_Destroy(AudioCache *cache);
AudioSample* Cache_GetSample(AudioCache *cache, const char *name);
void Cache_AddSample(AudioCache *cache, AudioSample *sample);
void Cache_RemoveSample(AudioCache *cache, AudioSample *sample);
int AudioCacheItemLock(AudioCacheItem *item);
void AudioCacheItemUnlock(AudioCacheItem *item);
int AudioCacheItemInUse(AudioCacheItem *item);
void AudioCacheItemFree(AudioCacheItem *item);
AudioSample* AudioCacheItemSample(AudioCacheItem *item);

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

static inline int AudioCacheItemLock(AudioCacheItem *item) {
    if (item) {
        item->locked = TRUE;
        return 0;
    }
    return -1;
}

static inline void Cache_Destroy(AudioCache *cache) {
    if (cache) {
        List_Clear(cache->entries);
        free(cache->entries);
        free(cache);
    }
}

static inline int AudioCacheItemLock(AudioCacheItem *item) {
    if (item) {
        item->locked = TRUE;
        return 0;
    }
    return -1;
}

static inline void AudioCacheItemUnlock(AudioCacheItem *item) {
    if (item) {
        item->locked = FALSE;
    }
}

static inline int AudioCacheItemInUse(AudioCacheItem *item) {
    if (item) {
        return item->refCount > 0;
    }
    return FALSE;
}

static inline void AudioCacheItemFree(AudioCacheItem *item) {
    if (item) {
        if (item->sample) {
            Source_Destroy(item->sample);
        }
        free(item);
    }
}

static inline AudioSample* AudioCacheItemSample(AudioCacheItem *item) {
    return item ? item->sample : NULL;
}

#endif
