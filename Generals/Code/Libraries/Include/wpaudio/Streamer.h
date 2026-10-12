#ifndef _WPAUDIO_STREAMER_H_
#define _WPAUDIO_STREAMER_H_

#include <wpaudio/altypes.h>
#include <wpaudio/StreamBuffering.h>

// Audio streamer
typedef struct {
    StreamBuffer buffer;
    ALboolean playing;
    ALuint position;
} AudioStreamer;

static inline void Streamer_Init(AudioStreamer *streamer, ALuint bufferSize) {
    StreamBuffer_Init(&streamer->buffer, bufferSize);
    streamer->playing = AL_FALSE;
    streamer->position = 0;
}

static inline void Streamer_Play(AudioStreamer *streamer) {
    if (streamer) streamer->playing = AL_TRUE;
}

static inline void Streamer_Stop(AudioStreamer *streamer) {
    if (streamer) streamer->playing = AL_FALSE;
}

#endif
