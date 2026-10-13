// wpaudio/cache.h - Audio cache management
// Original Westwood Studios implementation

#ifndef _WPAUDIO_CACHE_H_
#define _WPAUDIO_CACHE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/memory.h>
#include <wpaudio/lock.h>
#include <wpaudio/source.h>
#include <wpaudio/profiler.h>

// Forward declarations
struct AudioCache;
struct File;

// Callback to open an asset file
typedef File *AudioCacheOpenCB(const char *name);

// Audio cache - manages loaded audio samples
typedef struct AudioCache {
    int frameSize;
    AudioMemoryPool *framePool;
    AudioMemoryPool *itemPool;
    ListHead items;
    File *assetFile;
    AudioFormat assetFormat;
    int assetBytesLeft;
    AudioCacheOpenCB *openAssetCB;
    ProfileData profile;
} AudioCache;

// Audio cache item - a single cached sample
// nd MUST be first member for circular list traversal via cast
typedef struct AudioCacheItem {
    ListNode nd;
    int valid;
    const char *name;
    struct AudioCache *cache;
    Lock lock;
    AudioSample sample;
    AudioFormat format;
} AudioCacheItem;

// Implemented in AUD_Cache.cpp
AudioCache *AudioCacheCreate(int cacheSize, int maxItems, int frameSize);
void AudioCacheDestroy(AudioCache *cache);
AudioCacheItem *AudioCacheGetItem(AudioCache *cache, const char *name);
void AudioCacheInvalidate(AudioCache *cache);
AudioCacheOpenCB *AudioCacheSetOpenCB(AudioCache *cache, AudioCacheOpenCB *cb);
AudioCacheItem *AudioCacheLoadItem(AudioCache *cache, const char *name);
int AudioCacheFreeOldestItem(AudioCache *cache);

void AudioCacheItemLock(AudioCacheItem *item);
void AudioCacheItemUnlock(AudioCacheItem *item);
int AudioCacheItemInUse(AudioCacheItem *item);
void AudioCacheItemFree(AudioCacheItem *item);
AudioSample *AudioCacheItemSample(AudioCacheItem *item);

#endif // _WPAUDIO_CACHE_H_
