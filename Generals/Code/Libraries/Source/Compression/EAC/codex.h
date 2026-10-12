// EAC/codex.h - Compression codex interface

#ifndef __EAC_CODEX_H__
#define __EAC_CODEX_H__

#include "Lib/basetype.h"

typedef struct {
    int (*Compress)(void *dest, int *destLen, const void *source, int sourceLen);
    int (*Decompress)(void *dest, int *destLen, const void *source, int sourceLen);
} Codex;

extern Codex g_CodexTable[];

#endif // __EAC_CODEX_H__
