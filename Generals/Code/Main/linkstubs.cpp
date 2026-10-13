// linkstubs.cpp - empty/weak definitions so RTS.exe links as a milestone binary.
// Real implementations land as SDK backends are finished.
#include <stdint.h>
#include <stddef.h>
#include <wchar.h>

// --- zlib ---
extern "C" int compress2(unsigned char*, unsigned long*, const unsigned char*, unsigned long, int) { return 0; }
extern "C" unsigned long compressBound(unsigned long n) { return n + 64; }
extern "C" int uncompress(unsigned char*, unsigned long*, const unsigned char*, unsigned long) { return 0; }

// --- LZHL / BTREE / HUFF / REF ---
// Declare with asm labels first so definitions emit the expected mangled names.
extern "C" {
void* LZHLCreateCompressor() __asm__("_Z20LZHLCreateCompressorv");
void LZHLDestroyCompressor(void*) __asm__("_Z21LZHLDestroyCompressorPv");
unsigned LZHLCompressorCalcMaxBuf(unsigned) __asm__("_Z24LZHLCompressorCalcMaxBufj");
int LZHLCompress(void*, void*, const void*, unsigned) __asm__("_Z12LZHLCompressPvS_PKvj");
void* LZHLCreateDecompressor() __asm__("_Z22LZHLCreateDecompressorv");
void LZHLDestroyDecompressor(void*) __asm__("_Z23LZHLDestroyDecompressorPv");
int LZHLDecompress(void*, void*, unsigned*, const void*, unsigned) __asm__("_Z14LZHLDecompressPvS_PjPKvS0_");
int BTREE_encode(void*, const void*, int) __asm__("_Z12BTREE_encodePvPKvi");
int BTREE_decode(void*, void*, int) __asm__("_Z12BTREE_decodePvS_i");
int HUFF_encode(void*, const void*, int) __asm__("_Z11HUFF_encodePvPKvi");
int HUFF_decode(void*, void*, int) __asm__("_Z11HUFF_decodePvS_i");
int REF_encode(void*, const void*, int) __asm__("_Z10REF_encodePvPKvi");
int REF_decode(void*, void*, int) __asm__("_Z10REF_decodePvS_i");

// Definitions immediately after asm-labeled declarations
void* LZHLCreateCompressor() { return 0; }
void LZHLDestroyCompressor(void*) {}
unsigned LZHLCompressorCalcMaxBuf(unsigned n) { return n + 64; }
int LZHLCompress(void*, void*, const void*, unsigned) { return 0; }
void* LZHLCreateDecompressor() { return 0; }
void LZHLDestroyDecompressor(void*) {}
int LZHLDecompress(void*, void*, unsigned*, const void*, unsigned) { return 0; }
int BTREE_encode(void*, const void*, int) { return 0; }
int BTREE_decode(void*, void*, int) { return 0; }
int HUFF_encode(void*, const void*, int) { return 0; }
int HUFF_decode(void*, void*, int) { return 0; }
int REF_encode(void*, const void*, int) { return 0; }
int REF_decode(void*, void*, int) { return 0; }
}

// --- MinGW tchar import thunks ---
extern "C" {
size_t tcsclen(const wchar_t* s) { return s ? wcslen(s) : 0; }
int tcscmp(const wchar_t* a, const wchar_t* b) { return wcscmp(a, b); }
int tcsicmp(const wchar_t* a, const wchar_t* b) { return _wcsicmp(a, b); }
size_t (*__imp__tcsclen)(const wchar_t*) = tcsclen;
int (*__imp__tcscmp)(const wchar_t*, const wchar_t*) = tcscmp;
int (*__imp__tcsicmp)(const wchar_t*, const wchar_t*) = tcsicmp;
}
