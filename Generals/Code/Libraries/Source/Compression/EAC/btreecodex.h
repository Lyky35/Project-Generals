// EAC/btreecodex.h - BTree compression codex

#ifndef __EAC_BTREECODEX_H__
#define __EAC_BTREECODEX_H__

#include "EAC/codex.h"

extern Codex g_BTreeCodex;

int BTREE_encode(void *dest, const void *source, int sourceLen);
int BTREE_decode(void *dest, void *source, int sourceLen);

#endif // __EAC_BTREECODEX_H__
