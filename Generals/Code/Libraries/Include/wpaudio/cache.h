#ifndef _WPAUDIO_CACHE_H_
#define _WPAUDIO_CACHE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/source.h>
#include <wpaudio/profiler.h>

// Forward declarations
struct AudioCache;

// Audio frame for streaming
typedef struct AudioFrame {
    AudioSample *sample;
    void *Data;
    ALuint Bytes;
    ALuint sample_index;
    struct ListNode nd;
} AudioFrame;

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
    struct AudioCache *cache;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
} AudioCacheItem;

// Audio cache
typedef struct AudioCache {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
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

// Profiler cache functions
void ProfCacheRemove(ProfCache *prof, ALuint bytes);
void ProfCacheRemovePage(ProfCache *prof);
void AudioFrameDeinit(AudioFrame *frame);
void MemoryPoolReturnItem(MemoryPool *pool, void *item);
void AudioSampleDeinit(AudioSample *sample);
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
    struct AudioCache *cache;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
} AudioCacheItem;

// Audio cache
typedef struct AudioCache {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
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

// Profiler cache functions
void ProfCacheRemove(ProfCache *prof, ALuint bytes);
void ProfCacheRemovePage(ProfCache *prof);
void AudioFrameDeinit(AudioFrame *frame);
void MemoryPoolReturnItem(MemoryPool *pool, void *item);
void AudioSampleDeinit(AudioSample *sample);
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
    struct AudioCache *cache;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
} AudioCacheItem;

// Audio cache
typedef struct AudioCache {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
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

// Profiler cache functions
void ProfCacheRemove(ProfCache *prof, ALuint bytes);
void ProfCacheRemovePage(ProfCache *prof);
void AudioFrameDeinit(AudioFrame *frame);
void MemoryPoolReturnItem(MemoryPool *pool, void *item);
void AudioSampleDeinit(AudioSample *sample);
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Profiler cache stubs
static inline void ProfCacheRemove(ProfCache *prof, ALuint bytes) {
    (void)prof; (void)bytes;
}

static inline void ProfCacheRemovePage(ProfCache *prof) {
    (void)prof;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
    struct AudioCache *cache;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
} AudioCacheItem;

// Audio cache
typedef struct AudioCache {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
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

// Profiler cache functions
void ProfCacheRemove(ProfCache *prof, ALuint bytes);
void ProfCacheRemovePage(ProfCache *prof);
void AudioFrameDeinit(AudioFrame *frame);
void MemoryPoolReturnItem(MemoryPool *pool, void *item);
void AudioSampleDeinit(AudioSample *sample);
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Profiler cache stubs
static inline void ProfCacheRemove(ProfCache *prof, ALuint bytes) {
    (void)prof; (void)bytes;
}

static inline void ProfCacheRemovePage(ProfCache *prof) {
    (void)prof;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Audio cache item
typedef struct AudioCacheItem {
    AudioSample *sample;
    int refCount;
    int locked;
    int priority;
    struct ListNode nd;
    struct AudioCache *cache;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
} AudioCacheItem;

// Audio cache
typedef struct AudioCache {
    List *entries;
    ALuint maxSize;
    ALuint currentSize;
    struct ProfCache *profile;
    struct MemoryPool *framePool;
    struct MemoryPool *itemPool;
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

// Profiler cache functions
void ProfCacheRemove(ProfCache *prof, ALuint bytes);
void ProfCacheRemovePage(ProfCache *prof);
void AudioFrameDeinit(AudioFrame *frame);
void MemoryPoolReturnItem(MemoryPool *pool, void *item);
void AudioSampleDeinit(AudioSample *sample);
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

static inline AudioCache* Cache_Create(ALuint maxSize) {
    AudioCache *cache = (AudioCache*)malloc(sizeof(AudioCache));
    if (cache) {
        cache->entries = (List*)malloc(sizeof(List));
        List_Init(cache->entries);
        cache->maxSize = maxSize;
        cache->currentSize = 0;
        cache->profile = NULL;
        cache->framePool = NULL;
        cache->itemPool = NULL;
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

// Profiler cache stubs
static inline void ProfCacheRemove(ProfCache *prof, ALuint bytes) {
    (void)prof; (void)bytes;
}

static inline void ProfCacheRemovePage(ProfCache *prof) {
    (void)prof;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Profiler cache stubs
static inline void ProfCacheRemove(ProfCache *prof, ALuint bytes) {
    (void)prof; (void)bytes;
}

static inline void ProfCacheRemovePage(ProfCache *prof) {
    (void)prof;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Memory pool stub
typedef struct MemoryPool {
    void *data;
    int itemSize;
    int count;
} MemoryPool;

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

// Profiler cache stubs
static inline void ProfCacheRemove(ProfCache *prof, ALuint bytes) {
    (void)prof; (void)bytes;
}

static inline void ProfCacheRemovePage(ProfCache *prof) {
    (void)prof;
}

static inline void AudioFrameDeinit(AudioFrame *frame) {
    if (frame) free(frame);
}

static inline void MemoryPoolReturnItem(MemoryPool *pool, void *item) {
    (void)pool; (void)item;
}

static inline void AudioSampleDeinit(AudioSample *sample) {
    if (sample) {
        sample->id = 0;
        sample->buffer = 0;
    }
}

static inline void DBG_INVALIDATE_TYPE(void *ptr) {
    DBG_SET_TYPE(ptr, void)
}

#endif
