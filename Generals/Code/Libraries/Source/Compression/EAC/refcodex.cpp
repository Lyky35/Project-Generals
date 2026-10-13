#include "EAC/refcodex.h"

Codex g_RefCodex = { REF_encode, REF_decode };

int REF_encode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}

int REF_decode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}
