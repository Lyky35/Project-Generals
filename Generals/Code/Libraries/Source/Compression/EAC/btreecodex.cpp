#include "EAC/btreecodex.h"

Codex g_BTreeCodex = { BTREE_encode, BTREE_decode };

int BTREE_encode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}

int BTREE_decode(void *dest, int *destLen, const void *source, int sourceLen) {
    (void)dest; (void)destLen; (void)source; (void)sourceLen;
    return 0;
}

CODEXABOUT btree_about_data;
