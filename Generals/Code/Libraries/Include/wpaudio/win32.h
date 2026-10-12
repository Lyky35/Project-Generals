#ifndef _WPAUDIO_WIN32_H_
#define _WPAUDIO_WIN32_H_

// Win32 compatibility
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Win32 types
typedef DWORD DWORD;
typedef HANDLE HANDLE;
typedef HWND HWND;
typedef HINSTANCE HINSTANCE;

// Win32 functions (stubs for cross-platform)
#define GetCurrentThreadId() GetCurrentThreadId()
#define GetCurrentProcessId() GetCurrentProcessId()

#endif
