#ifndef _WPAUDIO_CHANNEL_H_
#define _WPAUDIO_CHANNEL_H_

#include <wpaudio/altypes.h>
#include <wpaudio/source.h>
#include <wpaudio/attributes.h>

// Forward declarations
struct AudioControl;
struct AudioAttribs;
struct AudioSample;
struct AudioFormat;
struct AudioDevice;
struct AudioDriver;

// Audio frame for streaming
typedef struct AudioFrame {
    AudioSample *sample;
    void *Data;
    ALuint Bytes;
    ALuint sample_index;
} AudioFrame;

// Audio channel
typedef struct AudioChannel {
    // Linked list node
    struct {
        struct AudioChannel *next;
        struct AudioChannel *prev;
    } nd;
    
    AudioControl Control;
    AudioAttribs attribs;
    
    struct AudioDevice *Device;
    struct AudioDriver *driver;
    AudioFormat current_format;
    int format_changed;
    int drv_format_changed;
    char sample_name[256];
    
    // Driver callbacks
    void (*drvCBNextFrame)(struct AudioChannel*);
    void (*drvCBNextSample)(struct AudioChannel*);
    void (*drvCBSampleDone)(struct AudioChannel*);
    
    // User callbacks
    void (*CB_NextFrame)(struct AudioChannel*);
    void (*CB_NextSample)(struct AudioChannel*);
    void (*CB_SampleDone)(struct AudioChannel*);
    void (*CB_Stop)(struct AudioChannel*);
    
    void *Data;
    int Type;
    
    AudioControl Control;
    AudioAttribs ChannelAttribs;
    AudioSample *sample;
    AudioAttribs *SfxAttribs;
    AudioAttribs *GroupAttribs;
    AudioAttribs *CompAttribs;
    AudioAttribs *FadeAttribs;
    
    // Streaming frame
    AudioFrame *frame;
    void *frameData;
    int bytesInFrame;
    int bytesRemaining;
    
    // User callbacks
    void (*CB_NextFrame)(struct AudioChannel*);
    void (*CB_NextSample)(struct AudioChannel*);
    void (*CB_SampleDone)(struct AudioChannel*);
    void (*CB_Stop)(struct AudioChannel*);
    
    // Control
    struct AudioControl Control;
    AudioAttribs GroupAttribs;
    AudioAttribs *SfxAttribs;
    AudioAttribs *CompAttribs;
    AudioAttribs *FadeAttribs;
    
    // Channel state
    int bytesInFrame;
    int bytesRemaining;
    void *frameData;
    
    // Callbacks
    void (*CB_NextFrame)(struct AudioChannel*);
    void (*CB_NextSample)(struct AudioChannel*);
    void (*CB_SampleDone)(struct AudioChannel*);
    void (*CB_Stop)(struct AudioChannel*);
} AudioChannel;

// Audio control
typedef struct AudioControl {
    int LoopCount;
    int Status;
    int Priority;
    int LoopStart;
    int LoopEnd;
    int Pan;
    int Volume;
    int Frequency;
} AudioControl;

// Channel state constants
#define AUDIO_CHANNEL_TYPE_STD 0
#define AUDIO_CHANNEL_TYPE_3D 1

// Control status flags
#define mAUDIO_CTRL_PLAYING 0x0001
#define mAUDIO_CTRL_PAUSED  0x0002
#define mAUDIO_CTRL_LOOPED  0x0004

// Priorities
#define AUD_LOW_PRIORITY 0
#define AUD_NORMAL_PRIORITY 1
#define AUD_HIGH_PRIORITY 2

// Boolean values
#define TRUE 1
#define FALSE 0

// Function declarations (implemented in AUD_Channel.cpp)
AudioChannel* Channel_Create();
void Channel_Destroy(AudioChannel *ch);
void Channel_Play(AudioChannel *ch, ALuint buffer);
void Channel_Stop(AudioChannel *ch);
void Channel_SetVolume(AudioChannel *ch, float vol);
void Channel_SetPan(AudioChannel *ch, float pan);
void Channel_SetFrequency(AudioChannel *ch, ALuint freq);
void AudioControlInit(AudioControl *ctrl);
int AudioChannelSetSample(AudioChannel *chan, AudioSample *sample);
int AudioChannelSetFormat(AudioChannel *chan, AudioFormat *new_format);
int AudioChannelIsAudible(AudioChannel *chan);

#endif // _WPAUDIO_CHANNEL_H_
