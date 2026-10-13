// asimp3/mss.h - Miles Sound System stub for cross-compilation
// Replaced Miles SDK with OpenAL + FFmpeg MP3 decoding

#ifndef _ASIMP3_MSS_H_
#define _ASIMP3_MSS_H_

#include <stdint.h>
#include <stdlib.h>

// Basic types
typedef uint32_t U32;
typedef int32_t S32;
typedef uint16_t U16;
typedef int16_t S16;
typedef uint8_t U8;
typedef int8_t S8;

// Calling conventions (no-ops on modern systems)
#ifndef FAR
#define FAR
#endif
#ifndef AILCALL
#define AILCALL
#endif
#ifndef AILCALLBACK
#define AILCALLBACK
#endif
#ifndef DXDEC
#define DXDEC
#endif

// ASI (Audio Streaming Interface) - MP3 decoding stream
typedef void *HASISTREAM;
typedef struct ASISTREAM {
    void *impl;
    S32 output_cursor;
    S32 frame_size;
    S32 input_cursor;
    S32 total_bytes;
} ASISTREAM;

// Callback for stream data fetching
typedef S32 (*ASISTREAM_CB)(U32 user, void *dest, S32 bytes, S32 offset);

// ASI functions (stubs - MP3 decoding handled by FFmpeg)
static inline int ASI_startup(void) { return 1; }
static inline void ASI_shutdown(void) {}
static inline HASISTREAM ASI_stream_open(U32 user, ASISTREAM_CB cb, S32 param) { (void)user; (void)cb; (void)param; return NULL; }
static inline S32 ASI_stream_process(HASISTREAM stream, void *dst, S32 dst_bytes) { (void)stream; (void)dst; (void)dst_bytes; return 0; }
static inline void ASI_stream_close(HASISTREAM stream) { (void)stream; }

// Note: AIL_mem_alloc_lock, AIL_mem_free_lock, AIL_MMX_available are
// defined in AUD_DSoundDriver.cpp as stubs - do not redefine here.

#endif // _ASIMP3_MSS_H_
