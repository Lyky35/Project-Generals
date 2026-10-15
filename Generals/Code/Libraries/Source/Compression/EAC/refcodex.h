// EAC/refcodex.h - Reference compression codex

#ifndef __EAC_REFCODEX_H__
#define __EAC_REFCODEX_H__

#include "EAC/codex.h"

extern Codex g_RefCodex;

// Signatures must match EAC/refencode.cpp + EAC/refdecode.cpp (the previous
// decls here matched neither, so the linker happily bound calls to the
// no-op REF_decode stub in Main/linkstubs.cpp - compressed files, including
// the shell map (.map, EAR/RefPack), could never be decompressed).
int REF_encode(void *compresseddata, const void *source, int sourcesize, int *opts);
int REF_decode(void *dest, const void *compresseddata, int *compressedsize);

#endif // __EAC_REFCODEX_H__
