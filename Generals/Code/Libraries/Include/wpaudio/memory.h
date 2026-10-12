// wpaudio/memory.h - Emulation stub for WPAudio memory functions

#ifndef _WPAUDIO_MEMORY_H_
#define _WPAUDIO_MEMORY_H_

#include <stdlib.h>

// Basic memory functions
#define DbgMalloc(size) malloc(size)
#define DbgFree(ptr) free(ptr)
#define DbgRealloc(ptr, size) realloc(ptr, size)

#endif // _WPAUDIO_MEMORY_H_
