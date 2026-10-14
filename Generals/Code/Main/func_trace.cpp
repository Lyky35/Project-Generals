// Optional whole-program function tracing (build with -DRTS_FUNCTION_TRACE=ON).
// Every instrumented function entry calls __cyg_profile_func_enter; when the
// environment variable RTS_FUNC_TRACE is set, entries are appended to
// RTS_FUNC.LOG next to the exe as "E <hex-address>". Map addresses back to
// names offline with: nm -C -n RTS.exe + addr2line.
#ifdef RTS_FUNCTION_TRACE_BUILD

#include <windows.h>

extern "C" void __cyg_profile_func_enter(void *fn, void *caller)
{
	(void)caller;
	static int enabled = -1;
	static HANDLE file = INVALID_HANDLE_VALUE;
	if (enabled < 0)
	{
		char env[8] = { 0 };
		enabled = (GetEnvironmentVariableA("RTS_FUNC_TRACE", env, sizeof(env)) > 0) ? 1 : 0;
	}
	if (!enabled)
		return;
	if (file == INVALID_HANDLE_VALUE)
	{
		char path[MAX_PATH] = { 0 };
		GetModuleFileNameA(NULL, path, MAX_PATH);
		char *slash = strrchr(path, '\\');
		if (slash)
			strcpy(slash + 1, "RTS_FUNC.LOG");
		else
			strcpy(path, "RTS_FUNC.LOG");
		file = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	if (file != INVALID_HANDLE_VALUE)
	{
		char buf[48];
		int pos = 0;
		buf[pos++] = 'E';
		buf[pos++] = ' ';
		unsigned long long v = (unsigned long long)(uintptr_t)fn;
		for (int shift = 60; shift >= 0; shift -= 4)
		{
			int nib = (int)((v >> shift) & 0xF);
			buf[pos++] = (char)(nib < 10 ? '0' + nib : 'a' + nib - 10);
		}
		buf[pos++] = '\r';
		buf[pos++] = '\n';
		DWORD written = 0;
		WriteFile(file, buf, (DWORD)pos, &written, NULL);
	}
}

extern "C" void __cyg_profile_func_exit(void *fn, void *caller)
{
	(void)fn;
	(void)caller;
}

#endif // RTS_FUNCTION_TRACE_BUILD
