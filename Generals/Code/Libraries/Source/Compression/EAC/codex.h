// EAC/codex.h - Compression codex interface

#ifndef __EAC_CODEX_H__
#define __EAC_CODEX_H__

#include "Lib/basetype.h"

typedef int (*CompressFunc)(void *dest, int *destLen, const void *source, int sourceLen);
typedef int (*DecompressFunc)(void *dest, int *destLen, const void *source, int sourceLen);

typedef struct {
    CompressFunc Compress;
    DecompressFunc Decompress;
} Codex;

extern Codex g_CodexTable[];

#endif // __EAC_CODEX_H__
