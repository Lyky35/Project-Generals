// wpaudio/altypes.h - Emulation stub for original project types
// Ported to OpenAL + SDL2 from Miles Audio / DirectSound SDK

#ifndef _WPAUDIO_ALTYPES_H_
#define _WPAUDIO_ALTYPES_H_

// Prevent system header conflicts
#if defined(__MINGW32__) || defined(__MINGW64__)
    #define WPAUDIO_MINGW 1
#endif

#include <AL/al.h>
#include <AL/alc.h>
#include <SDL2/SDL.h>

// Only define types that don't conflict with system headers
#ifndef ssize_t
    #if defined(WPAUDIO_MINGW)
        typedef long long ssize_t;
    #else
        typedef ssize_t ssize_t;
    #endif
#endif

#ifndef time_t
    #if defined(WPAUDIO_MINGW)
        typedef long long time_t;
    #else
        typedef time_t time_t;
    #endif
#endif

// OpenAL types emulation
typedef ALenum ALCenum;
typedef ALboolean ALCboolean;
typedef ALchar ALCchar;
typedef ALvoid ALCvoid;
typedef ALshort ALCshort;
typedef ALint ALCint;
typedef ALfloat ALCfloat;
typedef ALdouble ALCdouble;
typedef ALuint ALCuint;

// SDL types emulation
typedef Sint8 S8;
typedef Sint16 S16;
typedef Sint32 S32;
typedef Uint8 U8;
typedef Uint16 U16;
typedef Uint32 U32;
typedef Uint64 U64;

// Fixed-point types emulation
typedef S32 FIXED;
typedef U32 FRAC;

// DS3D types emulation (replaced with OpenAL)
typedef ALboolean DSCAPS2;
typedef ALboolean DS3DALCAPS;

// Buffer capabilities emulation
typedef struct {
    ALboolean fVcmPlaying;
    ALboolean fVcmRecord;
    ALboolean fCanPlay;
} DSBCAPS;

// Waveform format emulation (replaced with OpenAL format)
typedef struct {
    U16 wFormatTag;
    U16 nChannels;
    U32 nSamplesPerSec;
    U32 nAvgBytesPerSec;
    U16 nBlockAlign;
    U16 wBitsPerSample;
    U16 cbSize;
} DSBFORMAT;

// Primary buffer emulation
typedef struct {
    ALuint dwFlags;
    ALuint dwBufferLength;
    ALuint dwUnused;
    ALuint dwPlaybackPosition;
    ALuint dwCurrentPosition;
} DSBCAPS2;

// 3D audio emulation (replaced with OpenAL)
typedef struct {
    ALboolean dwFlags;
    ALfloat lMix;
    ALfloat rMix;
    ALfloat fDim;
} DSCAPS3D;

// Emulation: OpenAL device context
typedef struct {
    ALCdevice *device;
    ALCcontext *context;
} ALCcontext_settings;

// Emulation: Default audio format
#define AUF_DEFAULT_FORMAT (AL_FORMAT_STEREO_16)

// Emulation: Default buffer size
#define AUF_DEFAULT_BUFFER_SIZE (44100 * 2) // 2 seconds at 44.1kHz stereo

#endif // _WPAUDIO_ALTYPES_H_
