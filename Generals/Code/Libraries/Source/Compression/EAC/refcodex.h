// EAC/refcodex.h - Reference compression codex

#ifndef __EAC_REFCODEX_H__
#define __EAC_REFCODEX_H__

#include "EAC/codex.h"

extern Codex g_RefCodex;

int REF_encode(void *dest, const void *source, int sourceLen);
int REF_decode(void *dest, void *source, int sourceLen);

#endif // __EAC_REFCODEX_H__
