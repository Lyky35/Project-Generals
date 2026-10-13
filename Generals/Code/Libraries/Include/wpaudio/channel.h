// wpaudio/channel.h - Audio channel management
// Original Westwood Studios implementation

#ifndef _WPAUDIO_CHANNEL_H_
#define _WPAUDIO_CHANNEL_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/source.h>
#include <wpaudio/attributes.h>

// Forward declarations
struct AudioDevice;

// Audio driver - function pointers for hardware abstraction
struct AudioDriver {
    void *extended;           // extended data (NULL for standard)
    int (*openChannel)(struct AudioChannel *chan);
    void (*closeChannel)(struct AudioChannel *chan);
    int (*start)(struct AudioChannel *chan);
    void (*stop)(struct AudioChannel *chan);
    void (*pause)(struct AudioChannel *chan);
    void (*resume)(struct AudioChannel *chan);
    int (*check)(struct AudioChannel *chan);
    void (*update)(struct AudioChannel *chan);
    int (*queueIt)(struct AudioChannel *chan);
    int (*lock)(struct AudioChannel *chan);
    int (*unlock)(struct AudioChannel *chan);
};

// Channel types
typedef int AudioChannelType;
#define AUDIO_CHANNEL_TYPE_STD  0
#define AUDIO_CHANNEL_TYPE_3D   1
#define AUDIO_CHANNEL_TYPE_USER 2
#define AUDIO_CHANNEL_TYPE_ANY  0xFFFFFFFF
#define AUDIO_CHANNEL_TYPE_ALL  0xFFFFFFFF

// Callback type for channel events
typedef int AudioChannelCB(void *chan);

// Audio control - playback state
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

// Audio channel - a playback channel
typedef struct AudioChannel {
    ListNode nd;

    AudioControl Control;
    AudioAttribs attribs;

    struct AudioDevice *Device;
    struct AudioDriver *driver;
    AudioFormat current_format;
    int format_changed;
    int drv_format_changed;
    char sample_name[256];

    // Driver callbacks (called by driver during playback)
    AudioChannelCB *drvCBNextFrame;
    AudioChannelCB *drvCBNextSample;
    AudioChannelCB *drvCBSampleDone;

    // User callbacks (called by user code)
    AudioChannelCB *CB_NextFrame;
    AudioChannelCB *CB_NextSample;
    AudioChannelCB *CB_SampleDone;
    AudioChannelCB *CB_Stop;

    void *Data;
    int Type;

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

    // Driver-specific data
    void *drvData;
    int time_buffer;
    int time_min_frame;
} AudioChannel;

// Implemented in AUD_Channel.cpp
void AudioControlInit(AudioControl *ctrl);

AudioChannel *audioChannelCreate(AudioDevice *dev);
void audioChannelDestroy(AudioChannel *chan);
void audioChannelMakeStandard(AudioChannel *chan);
void audioChannelRecalcAttribs(AudioChannel *chan);

int AudioChannelTaken(AudioChannel *chan);
void AudioChannelReserve(AudioChannel *chan, AudioChannelType type);
void AudioChannelRelease(AudioChannel *chan);
void AudioChannelDestroy(AudioChannel *chan);
void AudioChannelStop(AudioChannel *chan);
void AudioChannelPause(AudioChannel *chan);
void AudioChannelResume(AudioChannel *chan);
void AudioChannelLock(AudioChannel *chan);
void AudioChannelUnlock(AudioChannel *chan);
void AudioChannelUse(AudioChannel *chan);
void AudioChannelNoUse(AudioChannel *chan);
int AudioChannelStart(AudioChannel *chan);
void AudioChannelSetSample(AudioChannel *chan, AudioSample *sample);
int AudioChannelSetFormat(AudioChannel *chan, AudioFormat *new_format);
int AudioChannelIsAudible(AudioChannel *chan);
void AudioChannelSetPitch(AudioChannel *chan, int pitch);
void AudioChannelSetVolume(AudioChannel *chan, int volume);
int AudioChannelGetVolume(AudioChannel *chan);
TimeStamp AudioChannelFrameTime(AudioChannel *chan);

#endif // _WPAUDIO_CHANNEL_H_
