// bink.h - Bink Video SDK stub (replaced by FFmpeg emulation)
#ifndef _BINK_H_
#define _BINK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* HBINK;
typedef void* HBINKBUFFER;

#define BINKTRACK_MASTERTRACK  ((uint32_t)0xffff)
#define BINKSURFACE_SINGLEBUFFERED 0
#define BINKSURFACE_DOUBLEBUFFERED 1
#define BINKSURFACE_TRIPLEBUFFERED 2
#define BINKSURFACE_QUADBUFFERED 3

typedef struct _BINK {
	uint32_t Width;
	uint32_t Height;
	uint32_t Frames;
	uint32_t FrameNum;
	uint32_t LastFrameNum;
	uint32_t FrameRate;
	uint32_t FrameRateDiv;
	uint32_t ReadError;
	uint32_t TrackMask;
	uint32_t NumTracks;
} BINK;

// Function stubs - return failure/no-op
static inline HBINK BinkOpen(const char* name, uint32_t flags) { (void)name; (void)flags; return 0; }
static inline int BinkClose(HBINK bink) { (void)bink; return 1; }
static inline int BinkDoFrame(HBINK bink) { (void)bink; return 0; }
static inline int BinkNextFrame(HBINK bink) { (void)bink; return 0; }
static inline int BinkWait(HBINK bink) { (void)bink; return 1; }
static inline int BinkCopyToBuffer(HBINK bink, void* dest, int32_t destpitch, uint32_t destheight, uint32_t destx, uint32_t desty, uint32_t flags) {
	(void)bink; (void)dest; (void)destpitch; (void)destheight; (void)destx; (void)desty; (void)flags; return 0;
}
static inline void BinkSetVolume(HBINK bink, int32_t track, int32_t volume) { (void)bink; (void)track; (void)volume; }
static inline int BinkGetError(void) { return 0; }

#ifdef __cplusplus
}
#endif

#endif // _BINK_H_
