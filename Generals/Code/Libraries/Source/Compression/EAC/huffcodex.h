// EAC/huffcodex.h - Huffman compression codex

#ifndef __EAC_HUFFCODEX_H__
#define __EAC_HUFFCODEX_H__

#include "EAC/codex.h"

extern Codex g_HuffCodex;

int HUFF_encode(void *dest, const void *source, int sourceLen);
int HUFF_decode(void *dest, void *source, int sourceLen);

#endif // __EAC_HUFFCODEX_H__
