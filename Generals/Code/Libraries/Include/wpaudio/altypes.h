// wpaudio/altypes.h - Base types, macros, and debug helpers
// Ported to OpenAL + SDL2 from Miles Audio / DirectSound SDK

#ifndef _WPAUDIO_ALTYPES_H_
#define _WPAUDIO_ALTYPES_H_

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include "bittype.h"

#ifndef uint64
typedef uint64_t uint64;
typedef int64_t int64;
#endif

#include <SDL2/SDL.h>

#include <AL/al.h>
#include <AL/alc.h>

// Basic types
typedef unsigned int uint;
typedef uint8_t uint8;
typedef int8_t int8;
typedef uint16_t uint16;
typedef int16_t int16;
// uint32, int32, uint64, int64 provided by <cstdint> or MinGW headers
typedef unsigned long ulong;
typedef long long longlong;
typedef unsigned short ushort;

typedef Sint8 S8;
typedef Sint16 S16;
typedef Sint32 S32;
typedef Uint8 U8;
typedef Uint16 U16;
typedef Uint32 U32;
typedef Uint64 U64;

// Fixed-point (FIXED conflicts with Windows SDK - guard it)
typedef Sint32 AudioFixed;
typedef Uint32 AudioFrac;
// FIXED type conflicts with wingdi.h - skip it
#ifndef FRAC
typedef Uint32 FRAC;
#endif

// Time stamp
typedef uint64 TimeStamp;

// Boolean
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// Memory macros
#define ALLOC_STRUCT(var, type) ((type *)calloc(1, sizeof(type)))

// AudioMemAlloc/AudioMemFree are wrappers around malloc/free
static inline void *AudioMemAlloc(size_t size) { return malloc(size); }
static inline void AudioMemFree(void *ptr) { free(ptr); }

// DBG type system (stubs for debug builds)
#define DBG_DECLARE_TYPE(type)
#define DBG_SET_TYPE(ptr, type)
#define DBG_INVALIDATE_TYPE(ptr)
#define DBG_TYPE()
#define DBG_ASSERT(x) assert(x)
#define DBG_ASSERT_TYPE(ptr, type) assert(ptr)
#define DBG_MSGASSERT(cond, msg) assert(cond)
#define DBG_MSGASSERT_TYPE(cond, msg, type) assert(cond)

// Debug printf
#define DBGPRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)

// Debug assertions with message
#define msg_assert(cond, msg) assert(cond)
#define DBG_Function(name)
#define DBG_FUNCTION(name)

// Flags
#define FLAGS_SET(flags, bit) ((flags) |= (bit))
#define FLAGS_CLEAR(flags, bit) ((flags) &= ~(bit))
#define FLAGS_TEST(flags, bit) ((flags) & (bit))
#define FLAGS_CHECK(flags, bit) ((flags) & (bit))

// DBG_CODE - execute block only in debug builds
#define DBG_CODE(x) x
#define DBG_ASSERT_PTR(ptr) assert(ptr != NULL)

// Error codes
#define vNO_ERROR 0
#define ERROR_CODE_FAIL (-1)
#define ERROR_CODE_MISSING_DATA (-2)
#define ERROR_CODE_NO_CHANNEL (-3)
#ifndef ERROR_CODE_IN_USE
#define ERROR_CODE_IN_USE (-4)
#endif

// Audio control status flags
#define mAUDIO_CTRL_PLAYING    0x0001
#define mAUDIO_CTRL_PAUSED     0x0002
#define mAUDIO_CTRL_LOOPED     0x0004
#define mAUDIO_CTRL_INUSE      0x0008
#define mAUDIO_CTRL_ALLOCATED  0x0010
#define mAUDIO_CTRL_ACTIVE     0x0020

// Stream buffer size
#define STREAM_BUFSIZE (64 * 1024)

// Timer frequency (100 Hz for compatibility with original code)
#define TIMER_HZ 100

// Memory allocation macros
#define vMEM_ANY 0
#define mMEM_INTERNAL 0
#define MEM_ALLOC_STRUCT(var, type) ALLOC_STRUCT(var, type)
#define MEM_Free(ptr) free(ptr)

// Loop forever constant
#define AUDIO_CTRL_LOOP_FOREVER (-1)

// Priorities
#define AUD_LOW_PRIORITY     0
#define AUD_NORMAL_PRIORITY  1
#define AUD_HIGH_PRIORITY    2
#define AUD_HIGHEST_PRIORITY 3

// Debug assert functions (implemented in AUD_Assert.cpp)
#ifdef __cplusplus
extern "C" {
#endif
void _aud_assert(const char *expr, const char *file, int line, const char *reason);
void _aud_debug_printf(const char *format, ...);
void _assert_printf(const char *format, ...);
#ifdef __cplusplus
}
#endif

#endif // _WPAUDIO_ALTYPES_H_
