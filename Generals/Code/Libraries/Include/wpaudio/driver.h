#ifndef _WPAUDIO_DRIVER_H_
#define _WPAUDIO_DRIVER_H_

#include <wpaudio/altypes.h>
#include <wpaudio/channel.h>

// Audio driver structure
typedef struct AudioDriver {
    int initialized;
    int maxChannels;
    
    // Driver functions
    int (*open)(struct AudioDevice *dev);
    void (*close)(struct AudioDevice *dev);
    int (*start)(struct AudioChannel *chan);
    void (*stop)(struct AudioChannel *chan);
    void (*pause)(struct AudioChannel *chan);
    void (*resume)(struct AudioChannel *chan);
    void (*lock)(struct AudioChannel *chan);
    void (*unlock)(struct AudioChannel *chan);
    int (*getPosition)(struct AudioChannel *chan, int *pos);
    int (*setVolume)(struct AudioChannel *chan, int volume);
    int (*setPan)(struct AudioChannel *chan, int pan);
    int (*setFrequency)(struct AudioChannel *chan, int freq);
    void (*update)(struct AudioDriver *drv);
    void (*lock)(struct AudioDriver *drv);
    void (*unlock)(struct AudioDriver *drv);
    void (*closeChannel)(struct AudioChannel *chan);
} AudioDriver;

// Function declarations
AudioDriver* Driver_Create();
void Driver_Destroy(AudioDriver *drv);
int Driver_Open(AudioDriver *drv, struct AudioDevice *dev);
void Driver_Close(AudioDriver *drv);

#endif
