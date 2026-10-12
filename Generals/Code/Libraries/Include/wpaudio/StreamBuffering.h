#ifndef _WPAUDIO_STREAMBUFFERING_H_
#define _WPAUDIO_STREAMBUFFERING_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>

// Stream buffering
typedef struct {
    List *buffers;
    ALuint bufferSize;
    ALuint fillLevel;
} StreamBuffer;

static inline void StreamBuffer_Init(StreamBuffer *sb, ALuint bufferSize) {
    sb->buffers = (List*)malloc(sizeof(List));
    List_Init(sb->buffers);
    sb->bufferSize = bufferSize;
    sb->fillLevel = 0;
}

static inline void StreamBuffer_Destroy(StreamBuffer *sb) {
    if (sb) {
        List_Clear(sb->buffers);
        free(sb->buffers);
    }
}

static inline void StreamBuffer_Add(StreamBuffer *sb, void *data, ALuint size) {
    if (sb && data && size > 0) {
        List_Add(sb->buffers, data);
        sb->fillLevel += size;
    }
}

#endif
