// wpaudio/device.h - Audio device management
// Original Westwood Studios implementation

#ifndef _WPAUDIO_DEVICE_H_
#define _WPAUDIO_DEVICE_H_

#include <wpaudio/altypes.h>
#include <wpaudio/list.h>
#include <wpaudio/lock.h>
#include <wpaudio/source.h>
#include <wpaudio/attributes.h>
#include <wpaudio/channel.h>

// Forward declarations
struct AudioChannel;
struct AudioDriver;
struct AudioSystemMaster;

// Device flags
#define mAUDIO_DEV_DONT_SERVICE 0x0001

// System master flags
#define mAUDIO_SYSTEM_MASTER_LOADED 0x0001

// System properties
#define mAUDIO_SYS_PROP_NONE      0x0000
#define mAUDIO_SYS_PROP_EXCLUSIVE 0x0001

// Master initialization
#define AUDIO_SYSTEM_MASTER_INIT_STATE 0
#define AUDIO_STAMP_SYSTEM_MASTER 0x5359534D  // "SYSM"

// Error codes
#ifndef ERROR_CODE_IN_USE
#define ERROR_CODE_IN_USE (-4)
#endif

// Default device unit
#define AUDIO_DEVICE_DEFAULT (-1)

// Service interval info
typedef struct AudioServiceInfo {
    TimeStamp serviceInterval;
    TimeStamp mustServiceInterval;
    TimeStamp lastInterval;
    TimeStamp longestInterval;
    TimeStamp lastServiceTime;
    TimeStamp periodInterval;
    TimeStamp longestIntervalforPeriod;
    TimeStamp periodStart;
    TimeStamp longestReset;
    int count;
    int lastcount;
    int animPos;
    int missCount;
} AudioServiceInfo;

// Search iterator for walking system/device lists
typedef struct AudioSearchStruct {
    ListNode *item;
} AudioSearch;

// Device callback type
typedef int AudioDeviceCB(struct AudioDevice *dev);

// Audio device - an audio output device
typedef struct AudioDevice {
    ListNode nd;
    int Unit;
    int systemUnit;
    struct AudioSystem *system;
    Lock lock;
    Lock channelAccess;
    struct AudioDriver *driver;
    AudioFormat DefaultFormat;
    AudioFormat Format;
    AudioAttribs Attribs;
    AudioAttribs *GroupAttribs;
    ListHead channelList;
    ListHead attribsList;
    AudioServiceInfo attribsUpdate;
    AudioServiceInfo mixerUpdate;
    AudioDeviceCB *deviceHandler;
    int flags;
    int maxChannels;
    int channels;
    int frames;
    int frame_lag;
    int over_sample;
} AudioDevice;

// Audio system - manages multiple devices
typedef struct AudioSystem {
    ListNode nd;
    Lock lock;
    struct AudioSystemMaster *master;
    int numUnits;
    AudioDevice *unit[16];
} AudioSystem;

// Audio system master - top-level driver interface
typedef struct AudioSystemMaster {
    const char *Name;
    void *reserved;           // 0x0 - reserved field
    int flags;
    int Properties;
    int stamp;
    int (*load)(AudioSystem *system);
    void (*unload)(AudioSystem *system);
    int (*open)(AudioDevice *dev);
    void (*close)(AudioDevice *dev);
    struct AudioDriver *driver;
} AudioSystemMaster;

// Externals defined in AUD_Device.cpp
extern int audioInitialized;
extern AudioAttribs *audioStdChannelAttribs;

// Implemented in AUD_Device.cpp
void audioRemoveChannel(AudioChannel *chan);
void audioAddChannel(AudioDevice *dev, AudioChannel *chan);

void AudioServiceInfoInit(AudioServiceInfo *asi);
void AudioServiceSetInterval(AudioServiceInfo *asi, TimeStamp interval);
void AudioServiceSetMustServiceInterval(AudioServiceInfo *asi, TimeStamp interval);
void AudioServiceSetResetInterval(AudioServiceInfo *asi, TimeStamp interval);
int AudioServiceNeeded(AudioServiceInfo *asi, TimeStamp now);
void AudioServicePerform(AudioServiceInfo *asi, TimeStamp now);

void AudioDeviceSetDefaultFormat(AudioDevice *dev, AudioFormat *format);
void AudioDeviceAttribsRemoveAll(AudioDevice *dev);
int AudioDeviceDestroyAllChannels(AudioDevice *dev, AudioChannelType type);
void AudioDeviceClose(AudioDevice *dev);
void AudioDeviceDestroy(AudioDevice *dev);
AudioChannel *AudioDeviceFirstChannel(AudioDevice *dev, AudioSearch *sh);
AudioChannel *AudioDeviceNextChannel(AudioSearch *sh);

void AudioDestroyAllDevices(void);
void AudioUnloadAllSystems(void);

AudioSystem *AudioFirstSystem(AudioSearch *sh);
AudioSystem *AudioNextSystem(AudioSearch *sh);
AudioDevice *AudioFirstDevice(AudioSearch *sh);
AudioDevice *AudioNextDevice(AudioSearch *sh);
int AudioDeviceService(AudioDevice *dev);
AudioChannel *AudioDeviceReserveChannel(AudioDevice *dev, AudioChannelType newType);
void AudioServiceInfoDump(AudioServiceInfo *asi);

#endif // _WPAUDIO_DEVICE_H_
