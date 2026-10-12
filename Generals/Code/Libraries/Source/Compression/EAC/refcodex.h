// EAC/refcodex.h - Reference compression codex

#ifndef __EAC_REFCODEX_H__
#define __EAC_REFCODEX_H__

#include "EAC/codex.h"

extern Codex g_RefCodex;

int REF_encode(void *dest, int *destLen, const void *source, int sourceLen);
int REF_decode(void *dest, int *destLen, const void *source, int sourceLen);

typedef struct {
    const char *name;
    const char *version;
} CODEXABOUT;

extern CODEXABOUT CODEXABOUT;

#endif // __EAC_REFCODEX_H__
