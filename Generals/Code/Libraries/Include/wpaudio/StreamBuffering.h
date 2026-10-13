#ifndef _WPAUDIO_STREAMBUFFERING_H_
#define _WPAUDIO_STREAMBUFFERING_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/lock.h>
#include <wpaudio/memory.h>
#include <wsys/File.h>

// Access IDs
#define vSTM_ACCESS_ID_OUT 0
#define vSTM_ACCESS_ID_IN  1
#define vSTM_MAX_ACCESSORS 2

// Access modes
#define vSTM_ACCESS_MODE_UPDATE 0
#define vSTM_ACCESS_MAX_MODES   1

// Stream flags
#define mSTM_STREAM_FAKE       0x0001
#define mSTM_STREAM_RESET_DONE 0x0002

// Access flags
#define mSTM_ACCESS_TOP_OF_START 0x0001

// W_ macros (Westwood flag utilities)
#define W_FlagsSet(flags, bit)   ((flags) |= (bit))
#define W_FlagsClear(flags, bit) ((flags) &= ~(bit))
#define W_UINT_MAX 0xFFFFFFFFu

// Alignment macro
#define ALIGN_UPBY(x, a) ((uintptr_t)(((uintptr_t)(x) + ((uintptr_t)(a) - 1)) & ~((uintptr_t)(a) - 1)))

// STM result codes
#define STM_OK   0
#define STM_EOF  1
#define STM_FAIL (-1)

// Memory allocation with flags (3 args as used in this file)
#define MEM_ALLOC_STRUCT(var, type, flags) ((type *)calloc(1, sizeof(type)))
#ifndef MEM_Free
#define MEM_Free(ptr) free(ptr)
#endif

// Region within a buffer - one per access ID
typedef struct STM_REGION {
    void *Data;
    int Bytes;
} STM_REGION;

// Block structure - a chunk of buffer memory
typedef struct STM_BLOCK {
    void *Data;
    int Bytes;
} STM_BLOCK;

// Stream buffer - a single buffer in the ring
typedef struct STM_SBUFFER {
    ListNode nd;
    int id;
    void *buffer_memory;
    STM_BLOCK data_region;
    STM_REGION region[vSTM_MAX_ACCESSORS];
    struct STM_STREAM *stream;
    struct AudioFormat *format;
    int align;
} STM_SBUFFER;

// Stream access - read/write position tracking
typedef struct STM_ACCESS {
    int id;
    int mode;
    int flags;
    int position;
    int access_pos;
    int bytes_in;
    int bytes_out;
    int last_error;
    Lock in_service;
    Lock ref;
    STM_BLOCK Block;
    STM_SBUFFER *start_buffer;
    STM_SBUFFER *SBuffer;
    struct STM_STREAM *stream;
} STM_ACCESS;

// Stream profile - tracks throughput
typedef struct STM_PROFILE {
    int active;
    int bytes;
    TimeStamp startTime;
    TimeStamp endTime;
} STM_PROFILE;

// Stream - manages a ring of buffers for streaming audio
typedef struct STM_STREAM {
    ListHead buffer_list;
    int buffers;       // count of buffers in the ring
    int bytes;
    int flags;
    Lock ref;
    STM_ACCESS access[vSTM_MAX_ACCESSORS];
} STM_STREAM;

// Implemented in AUD_StreamBuffering.cpp
void STM_stream_init(STM_STREAM *stm);
void STM_stream_reset(STM_STREAM *stm);
void STM_sbuffer_init(STM_SBUFFER *buf);
void STM_sbuffer_reset(STM_SBUFFER *sbuf);
void STM_access_init(STM_STREAM *stm, STM_ACCESS *access, int id);
void STM_access_reset(STM_ACCESS *access, STM_SBUFFER *first);
void STM_access_return_to_start(STM_ACCESS *access);
STM_SBUFFER *STM_next_sbuffer(STM_SBUFFER *sbuf);

STM_STREAM *STM_StreamCreate(void);
void STM_StreamDestroy(STM_STREAM *stm);
void STM_StreamDestroyBuffers(STM_STREAM *stm);
void STM_StreamReset(STM_STREAM *stm);
void STM_StreamAddSBuffer(STM_STREAM *stm, STM_SBUFFER *sbuf);
int STM_StreamCreateSBuffers(STM_STREAM *stm, int num_buffers, int buf_size, int align);
STM_ACCESS *STM_StreamAcquireAccess(STM_STREAM *stm, int access_id);
uint STM_StreamTotalBytesIn(STM_STREAM *stm);
uint STM_StreamTotalBytesTillFull(STM_STREAM *stm);
uint STM_StreamTotalBytes(STM_STREAM *stm);
int STM_StreamFull(STM_STREAM *stm);

STM_SBUFFER *STM_SBufferCreate(int bytes, int align);
void STM_SBufferDestroy(STM_SBUFFER *sbuf);
void STM_SBufferRemove(STM_SBUFFER *sbuf);
STM_SBUFFER *STM_SBufferNext(STM_SBUFFER *sbuf);

void STM_AccessRelease(STM_ACCESS *access);
int STM_AccessSetMode(STM_ACCESS *access, int mode);
int STM_AccessMode(STM_ACCESS *access);
int STM_AccessID(STM_ACCESS *access);
int STM_AccessTransfer(STM_ACCESS *access, void *data, int bytes);
int STM_AccessFileTransfer(STM_ACCESS *access, File *file, int bytes, int *transfered);
int STM_AccessGetError(STM_ACCESS *access);
int STM_AccessUpdate(STM_ACCESS *access);
int STM_AccessAdvance(STM_ACCESS *access, int bytes_to_advance);
void STM_AccessReturnToStart(STM_ACCESS *access);
int STM_AccessNextBlock(STM_ACCESS *access);
int STM_AccessGetBlock(STM_ACCESS *access);
uint STM_AccessTotalBytes(STM_ACCESS *access);
uint STM_AccessPosition(STM_ACCESS *access);
STM_ACCESS *STM_AccessUpStreamAccessor(STM_ACCESS *access);

void STM_ProfileStart(STM_PROFILE *pf);
void STM_ProfileStop(STM_PROFILE *pf);
void STM_ProfileUpdate(STM_PROFILE *pf, int bytes);
int STM_ProfileResult(STM_PROFILE *pf);
int STM_ProfileActive(STM_PROFILE *pf);

#endif // _WPAUDIO_STREAMBUFFERING_H_
