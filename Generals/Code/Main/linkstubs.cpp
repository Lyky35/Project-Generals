// linkstubs.cpp - empty/weak definitions so RTS.exe links as a milestone binary.
// Real implementations land as SDK backends are finished.
#include <stdint.h>
#include <cstdio>
#include <windows.h>
#include <string.h>
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
}

// --- MinGW tchar import thunks ---
// tchar.h declares _tcscmp/_tcsicmp/_tcsclen as dllimports taking const char*
// (this is an ANSI/TCHAR=char build), but there is no such export in the MinGW
// import libs, so the thunks below satisfy the linker. They MUST be char
// routines: they used to call the WIDE ones (wcscmp/wcslen/_wcsicmp), which made
// every StringClass comparison of equal strings return garbage. That broke the
// WW3D texture hash cache (StringClass is its key type): the menu background
// missed the cache every frame, a fresh VkTexture was created and leaked each
// time (device memory exhaustion -> white screen) and the TGA was re-decoded
// every frame (~1 fps).
extern "C" {
size_t tcsclen(const char* s) { return s ? strlen(s) : 0; }
int tcscmp(const char* a, const char* b) { return strcmp(a, b); }
int tcsicmp(const char* a, const char* b) { return _stricmp(a, b); }
size_t (*__imp__tcsclen)(const char*) = tcsclen;
int (*__imp__tcscmp)(const char*, const char*) = tcscmp;
int (*__imp__tcsicmp)(const char*, const char*) = tcsicmp;
}

// --- Throw tracer (RTS_DEBUG_THROWS via --wrap=__cxa_throw) ---
// Logs every C++ throw with its (mangled) type name and caller address so a
// field/wine log can name the thrower; resolve the address with nm.
#include <typeinfo>
extern "C" void __real___cxa_throw(void *thrown_exception, void *tinfo, void (*dest)(void *));
extern "C" void __wrap___cxa_throw(void *thrown_exception, void *tinfo, void (*dest)(void *))
{
	const char *name = "?";
	if (tinfo)
		name = ((std::type_info *)tinfo)->name();
	FILE *lf = fopen("RTS_START.LOG", "a");
	if (lf) {
		fprintf(lf, "VK: cxa_throw type=%s from=%p\n", name, __builtin_return_address(0));
		fclose(lf);
	}
	__real___cxa_throw(thrown_exception, tinfo, dest);
}

// --- Time shims for static FFmpeg avutil (avoid libwinpthread) ---
#include <windows.h>
#include <string.h>
extern "C" long long clock_gettime64(void *tp) {
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	unsigned long long t = ((unsigned long long)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
	// timespec: tv_sec at 0 (8 bytes), tv_nsec at 8 (8 bytes on LP64, 8 on LLP64 long long)
	if (tp) {
		unsigned char *p = (unsigned char *)tp;
		long long sec = (long long)(t / 10000000ULL) - 11644473600ULL;
		long long nsec = (long long)((t % 10000000ULL) * 100);
		memcpy(p, &sec, 8);
		memcpy(p + 8, &nsec, 8);
	}
	return 0;
}
extern "C" int nanosleep64(const void *req, void *rem) {
	(void)req; (void)rem;
	return 0;
}
extern "C" unsigned long clock(void) { return (unsigned long)GetTickCount(); }
