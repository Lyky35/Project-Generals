// EAC/codex.h - Compression codex interface

#ifndef __EAC_CODEX_H__
#define __EAC_CODEX_H__

#include "Lib/basetype.h"

// Calling conventions
#ifndef GCALL
#define GCALL
#endif
#ifndef galloc
#define galloc malloc
#endif
#ifndef QMAKEID
#define QMAKEID(a,b,c,d) (((int)(a)<<24)|((int)(b)<<16)|((int)(c)<<8)|(int)(d))
#endif

// Codex about structure
typedef struct {
    int signature;
    int size;
    int version;
    int decode;
    int encode;
    int size32;
    char versionstr[16];
    char shorttypestr[8];
    char longtypestr[32];
} CODEXABOUT;

#include "gimex.h"

// Min/max macros
#ifndef qmin
#define qmin(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef qmax
#define qmax(a,b) ((a)>(b)?(a):(b))
#endif

typedef int (*CompressFunc)(void *dest, int *destLen, const void *source, int sourceLen);
typedef int (*DecompressFunc)(void *dest, int *destLen, const void *source, int sourceLen);

typedef struct {
    int (*Compress)(void *dest, int *destLen, const void *source, int sourceLen);
    int (*Decompress)(void *dest, int *destLen, const void *source, int sourceLen);
} Codex;

extern Codex g_CodexTable[];

#endif // __EAC_CODEX_H__
