// wpaudio/source.h - Audio sample, frame, and format
// Original Westwood Studios implementation

#ifndef _WPAUDIO_SOURCE_H_
#define _WPAUDIO_SOURCE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wsys/File.h>

// Forward declarations
struct AudioAttribs;
struct AudioFormat;
struct AudioSample;

// Compression types
#define AUDIO_COMPRESS_NONE      0
#define AUDIO_COMPRESS_IMA_ADPCM 1
#define AUDIO_COMPRESS_MS_ADPCM  2
#define AUDIO_COMPRESS_MP3       3
#define AUDIO_COMPRESS_MAX_ID    4

// MP3 header data
typedef struct {
    uint32 Header;
} AudioMP3CData;

// ADPCM format data
typedef struct {
    int BlockSize;
    int SamplesPerBlock;
    int NumCoef;
    int Coef1;
    int Coef2;
} AudioADPCMData;

// Format-specific data
typedef union {
    AudioMP3CData mp3;
    AudioADPCMData adpcm;
    uint8 raw[64];
} AudioFormatCData;

// Audio format description
typedef struct AudioFormat {
    int Channels;
    int SampleWidth;
    int Rate;
    int Compression;
    int BytesPerSecond;
    int Flags;
    AudioFormatCData cdata;
} AudioFormat;

// Audio frame - a chunk of sample data
typedef struct AudioFrame {
    AudioSample *sample;
    void *Data;
    int Bytes;
    ListNode nd;
} AudioFrame;

// Audio sample - a loaded sound
typedef struct AudioSample {
    int id;
    void *Data;
    int Bytes;
    AudioFormat *Format;
    struct AudioAttribs *Attribs;
    char name[256];
    ListHead Frames;
} AudioSample;

// Implemented in AUD_Source.cpp
AudioSample *AudioCreateSample(uint bytes);
void AudioSampleDestroy(AudioSample *sample);
void AudioSampleInit(AudioSample *sample);
void AudioSampleSetName(AudioSample *sample, const char *orig_name);
void AudioSampleAddFrame(AudioSample *sample, AudioFrame *frame);
AudioFrame *AudioSampleFirstFrame(AudioSample *sample);
void AudioSampleDeinit(AudioSample *sample);

int AudioFormatBytes(AudioFormat *format, TimeStamp time);
TimeStamp AudioFormatTime(AudioFormat *format, int bytes);
void AudioFormatInit(AudioFormat *format);
void AudioFormatUpdate(AudioFormat *format);
int AudioFormatReadMP3File(File *file, AudioFormat *format, int *datasize);
int AudioFormatSeekToPos(File *file, const AudioFormat *format, int pos, int data_start);
int AudioFormatSame(const AudioFormat *f1, const AudioFormat *f2);

void AudioFrameInit(AudioFrame *frame, void *data, int bytes);
void AudioFrameDeinit(AudioFrame *frame);

// Implemented in AUD_Windows.cpp
int AudioFormatReadWaveFile(File *file, AudioFormat *format, int *bytes);

#endif // _WPAUDIO_SOURCE_H_
