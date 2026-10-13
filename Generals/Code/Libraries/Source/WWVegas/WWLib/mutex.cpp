/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "mutex.h"
#include "wwdebug.h"
#include <windows.h>


// ----------------------------------------------------------------------------

MutexClass::MutexClass(const char* name) : handle(NULL), locked(false)
{
	// Deferred: CreateMutex on first Lock() — avoid heap/Win32 during static init.
	(void)name;
}

MutexClass::~MutexClass()
{
	WWASSERT(!locked); // Can't delete locked mutex!
	if (handle)
	{
		CloseHandle(handle);
		handle = NULL;
	}
}

bool MutexClass::Lock(int time)
{
	if (handle == NULL)
	{
		handle = CreateMutex(NULL, false, NULL);
		if (!handle)
			return false;
	}
	int res = WaitForSingleObject(handle, time == WAIT_INFINITE ? INFINITE : time);
	if (res != WAIT_OBJECT_0)
		return false;
	locked++;
	return true;
}

void MutexClass::Unlock()
{
	#ifdef _UNIX
		//assert(0);
	#else
		WWASSERT(locked);
		int res=ReleaseMutex(handle);
		res;	// silence compiler warnings
		WWASSERT(res);
		locked--;
	#endif
}

// ----------------------------------------------------------------------------

MutexClass::LockClass::LockClass(MutexClass& mutex_,int time) : mutex(mutex_)
{
	failed=!mutex.Lock(time);
}

MutexClass::LockClass::~LockClass()
{
	if (!failed) mutex.Unlock();
}







// ----------------------------------------------------------------------------

CriticalSectionClass::CriticalSectionClass() : handle(NULL), locked(false)
{
	// Defer InitializeCriticalSection + heap alloc until first Lock().
	// File-scope CriticalSectionClass objects are constructed during CRT static
	// init; allocating here (operator new / UCRT) has been crashing in ucrtbase
	// before WinMain on MinGW+UCRT.
}

CriticalSectionClass::~CriticalSectionClass()
{
	WWASSERT(!locked); // Can't delete locked mutex!
	if (handle)
	{
		DeleteCriticalSection((CRITICAL_SECTION*)handle);
		delete[] (char*)handle;
		handle = NULL;
	}
}

void CriticalSectionClass::Lock()
{
	if (handle == NULL)
	{
		// First use: create the underlying CRITICAL_SECTION (kernel32 only).
		handle = (void*)new char[sizeof(CRITICAL_SECTION)];
		InitializeCriticalSection((CRITICAL_SECTION*)handle);
	}
	EnterCriticalSection((CRITICAL_SECTION*)handle);
	locked++;
}

void CriticalSectionClass::Unlock()
{
	WWASSERT(locked);
	if (handle)
		LeaveCriticalSection((CRITICAL_SECTION*)handle);
	locked--;
}

// ----------------------------------------------------------------------------

CriticalSectionClass::LockClass::LockClass(CriticalSectionClass& critical_section) : CriticalSection(critical_section)
{
	CriticalSection.Lock();
}

CriticalSectionClass::LockClass::~LockClass()
{
	CriticalSection.Unlock();
}


