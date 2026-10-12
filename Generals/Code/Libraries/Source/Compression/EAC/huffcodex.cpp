#include "EAC/huffcodex.h"

Codex g_HuffCodex = { HUFF_encode, HUFF_decode };

int HUFF_encode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}

int HUFF_decode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}

CODEXABOUT CODEXABOUT = { "Huffman Compression", "1.0" };
