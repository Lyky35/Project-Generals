/*
** zlib_wrapper.h - Wrapper for zlib to provide z_compress2 and z_uncompress
*/

#ifndef __zlib_wrapper_h
#define __zlib_wrapper_h

// Undef Byte BEFORE including zlib/zconf to avoid conflict with BaseType.h
#undef Byte

#include "ZLib/zlib.h"

// Wrapper functions to match expected API
static inline int z_compress2(void *dest, uLongf *destLen, const void *source, uLong sourceLen, int level) {
    return compress2((Bytef*)dest, (uLongf*)destLen, (const Bytef*)source, (uLong)sourceLen, level);
}

static inline int z_uncompress(void *dest, uLongf *destLen, const void *source, uLong sourceLen) {
    return uncompress((Bytef*)dest, (uLongf*)destLen, (const Bytef*)source, (uLong)sourceLen);
}

#endif // __zlib_wrapper_h
