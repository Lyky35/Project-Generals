// EAC/btreecodex.h - BTree compression codex

#ifndef __EAC_BTREECODEX_H__
#define __EAC_BTREECODEX_H__

#include "EAC/codex.h"

extern Codex g_BTreeCodex;

int BTREE_encode(void *dest, int *destLen, const void *source, int sourceLen);
int BTREE_decode(void *dest, int *destLen, const void *source, int sourceLen);

// CODEXABOUT structure
typedef struct {
    const char *name;
    const char *version;
} CODEXABOUT;

extern CODEXABOUT CODEXABOUT;

#endif // __EAC_BTREECODEX_H__
