// rts_bootlog.h — kernel32-only boot breadcrumb (safe during CRT static init)
#ifndef RTS_BOOTLOG_H
#define RTS_BOOTLOG_H

#include <windows.h>
#include <string.h>

inline void rtsBootLog(const char *msg)
{
	char path[MAX_PATH];
	path[0] = 0;
	GetModuleFileNameA(NULL, path, MAX_PATH);
	char *slash = strrchr(path, '\\');
	if (slash) strcpy(slash + 1, "RTS_START.LOG");
	else strcpy(path, "RTS_START.LOG");
	HANDLE h = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
		OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h != INVALID_HANDLE_VALUE) {
		SetFilePointer(h, 0, NULL, FILE_END);
		DWORD n = 0;
		WriteFile(h, msg, (DWORD)lstrlenA(msg), &n, NULL);
		WriteFile(h, "\r\n", 2, &n, NULL);
		CloseHandle(h);
	}
}

#endif
