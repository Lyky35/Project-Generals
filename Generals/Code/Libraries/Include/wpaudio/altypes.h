// wpaudio/altypes.h - Emulation stub for original project types
// Ported to OpenAL + SDL2 from Miles Audio / DirectSound SDK

#ifndef _WPAUDIO_ALTYPES_H_
#define _WPAUDIO_ALTYPES_H_

#include <cassert>
#include <SDL2/SDL.h>

#if defined(__MINGW32__) || defined(__MINGW64__)
    #undef ssize_t
    #undef time_t
    #undef uintptr_t
    #define WPAUDIO_MINGW 1
#endif

#include <AL/al.h>
#include <AL/alc.h>

// Type definitions that match original project but avoid system conflicts
typedef Sint8 S8;
typedef Sint16 S16;
typedef Sint32 S32;
typedef Uint8 U8;
typedef Uint16 U16;
typedef Uint32 U32;
typedef Uint64 U64;

// Fixed-point types
typedef Sint32 FIXED;
typedef Uint32 FRAC;

// DS3D types emulation (replaced with OpenAL)
typedef ALboolean DSCAPS2;
typedef ALboolean DS3DALCAPS;
typedef ALboolean DSCAPS3D;

// Buffer capabilities
typedef struct {
    ALboolean fVcmPlaying;
    ALboolean fVcmRecord;
    ALboolean fCanPlay;
} DSBCAPS;

// Waveform format
typedef struct {
    Uint16 wFormatTag;
    Uint16 nChannels;
    Uint32 nSamplesPerSec;
    Uint32 nAvgBytesPerSec;
    Uint16 nBlockAlign;
    Uint16 wBitsPerSample;
    Uint16 cbSize;
} DSBFORMAT;

// Primary buffer
typedef struct {
    Uint32 dwFlags;
    Uint32 dwBufferLength;
    Uint32 dwUnused;
    Uint32 dwPlaybackPosition;
    Uint32 dwCurrentPosition;
} DSBCAPS2;

// 3D audio
typedef struct {
    ALboolean dwFlags;
    ALfloat lMix;
    ALfloat rMix;
    ALfloat fDim;
} DSCAPS3D;

// OpenAL context
typedef struct {
    ALCdevice *device;
    ALCcontext *context;
} ALCcontext_settings;

// Defaults
#define AUF_DEFAULT_FORMAT (AL_FORMAT_STEREO_16)
#define AUF_DEFAULT_BUFFER_SIZE (44100 * 2)

// Boolean
#define TRUE 1
#define FALSE 0

// DBG macros
#define DBG_ASSERT(x) assert(x)
#define DBG_ASSERT_TYPE(ptr, type) assert(ptr)
#define DBG_SET_TYPE(ptr, type)
#define DBG_MSGASSERT(cond, msg) assert(cond)
#define DBG_MSGASSERT_TYPE(cond, msg, type) assert(cond)
#define DBG_INVALIDATE_TYPE(ptr) DBG_SET_TYPE(ptr, void)

// Flags
#define FLAGS_SET(flags, bit) ((flags) |= (bit))
#define FLAGS_CLEAR(flags, bit) ((flags) &= ~(bit))
#define FLAGS_TEST(flags, bit) ((flags) & (bit))

// Error codes
#define vNO_ERROR AL_NO_ERROR

// Audio control status flags
#define mAUDIO_CTRL_PLAYING 0x0001
#define mAUDIO_CTRL_PAUSED  0x0002
#define mAUDIO_CTRL_LOOPED  0x0004

// Priorities
#define AUD_LOW_PRIORITY 0
#define AUD_NORMAL_PRIORITY 1
#define AUD_HIGH_PRIORITY 2

// Boolean
#define TRUE 1
#define FALSE 0

// Lock function
static inline int Locked(void *lock) {
    (void)lock;
    return TRUE;
}

#endif // _WPAUDIO_ALTYPES_H_
