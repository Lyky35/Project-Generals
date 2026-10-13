// wpaudio/memory.h - Memory pool allocation
// Original Westwood Studios implementation

#ifndef _WPAUDIO_MEMORY_H_
#define _WPAUDIO_MEMORY_H_

#include <wpaudio/altypes.h>

// Memory item header - stored before each allocated block
typedef struct MemoryItem {
    struct MemoryItem *next;
} MemoryItem;

// Audio memory pool - fixed-size block allocator
typedef struct AudioMemoryPool {
    MemoryItem *next;       // free list head
    MemoryItem *base;       // base of allocated memory
    int itemsOut;           // items currently allocated (debug)
    int numItems;           // total items in pool (debug)
    uint items;
    uint itemSize;
    uint count;
} AudioMemoryPool;

// Implemented in AUD_Memory.cpp
AudioMemoryPool *MemoryPoolCreate(uint items, uint size);
void MemoryPoolDestroy(AudioMemoryPool *pool);
void *MemoryPoolGetItem(AudioMemoryPool *pool);
void MemoryPoolReturnItem(AudioMemoryPool *pool, void *data);
int MemoryPoolCount(AudioMemoryPool *pool);

// String helpers (implemented in AUD_Memory.cpp)
void AudioAddSlash(char *string);
int AudioHasPath(const char *string);

#endif // _WPAUDIO_MEMORY_H_
