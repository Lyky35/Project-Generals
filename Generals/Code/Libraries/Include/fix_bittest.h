#pragma once
// MinGW's <winnt.h> does `#define BitTest _bittest` on x64 (the pointer +
// bit-index compiler intrinsic). This codebase's BitTest from BaseType.h is a
// VALUE mask test: `#define BitTest(x, i) (((x) & (i)) != 0)`. Any TU that
// pulls in <windows.h> after BaseType.h silently gets the intrinsic form and
// compiles BitTest(status, FLAG) as a memory bit-test of address `status`
// (crash reading e.g. 0x98). Force the windows headers to load first, then
// restore the value semantics for every translation unit.
#include <windows.h>
#include <stdlib.h>
// windows.h pulls in intrin.h, after which <stdlib.h> skips these prototypes;
// crc.h/crc.cpp need them.
extern "C" unsigned long _lrotl(unsigned long, int);
extern "C" unsigned long _lrotr(unsigned long, int);
#undef BitTest
#define BitTest( x, i ) ( ( (x) & (i) ) != 0 )
// Win32 GetFileType() constants clash with this codebase's FileTypes enum
// (W3DFileSystem.cpp uses FILE_TYPE_UNKNOWN/W3D/TGA/DDS).
#undef FILE_TYPE_UNKNOWN
#undef FILE_TYPE_DISK
#undef FILE_TYPE_CHAR
#undef FILE_TYPE_PIPE
