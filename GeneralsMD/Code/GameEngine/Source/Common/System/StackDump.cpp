/*
**	Command & Conquer Generals Zero Hour(tm)
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

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#if defined(_DEBUG) || defined(_INTERNAL) || defined(IG_DEBUG_STACKTRACE)

#pragma pack(push, 8)

#pragma comment(linker, "/defaultlib:Dbghelp.lib")

#include "Common/StackDump.h"
#include "Common/Debug.h"

#if defined(_M_IX86)
#define EXC_CTX_IP(c) ((c)->Eip)
#define EXC_CTX_SP(c) ((c)->Esp)
#define EXC_CTX_FP(c) ((c)->Ebp)
#define EXC_CTX_R0(c) ((c)->Eax)
#define EXC_CTX_R1(c) ((c)->Ebx)
#define EXC_CTX_R2(c) ((c)->Ecx)
#define EXC_CTX_R3(c) ((c)->Edx)
#define EXC_CTX_R4(c) ((c)->Esi)
#define EXC_CTX_R5(c) ((c)->Edi)
#define EXC_CTX_FLAGS(c) ((c)->EFlags)
#define EXC_IMAGE_MACHINE IMAGE_FILE_MACHINE_I386
#else
#define EXC_CTX_IP(c) ((c)->Rip)
#define EXC_CTX_SP(c) ((c)->Rsp)
#define EXC_CTX_FP(c) ((c)->Rbp)
#define EXC_CTX_R0(c) ((c)->Rax)
#define EXC_CTX_R1(c) ((c)->Rbx)
#define EXC_CTX_R2(c) ((c)->Rcx)
#define EXC_CTX_R3(c) ((c)->Rdx)
#define EXC_CTX_R4(c) ((c)->Rsi)
#define EXC_CTX_R5(c) ((c)->Rdi)
#define EXC_CTX_FLAGS(c) ((c)->EFlags)
#define EXC_IMAGE_MACHINE IMAGE_FILE_MACHINE_AMD64
#endif


//*****************************************************************************
//	Prototypes
//*****************************************************************************
BOOL InitSymbolInfo(void);
void UninitSymbolInfo(void);
void MakeStackTrace(DWORD_PTR myeip,DWORD_PTR myesp,DWORD_PTR myebp, int skipFrames, void (*callback)(const char*));
void GetFunctionDetails(void *pointer, char*name, char*filename, unsigned int* linenumber, unsigned int* address);
void WriteStackLine(void*address, void (*callback)(const char*));

//*****************************************************************************
//	Mis-named globals :-)
//*****************************************************************************
static CONTEXT gsContext;
static Bool gsInit=FALSE;

BOOL (__stdcall *gsSymGetLineFromAddr)(
		IN  HANDLE                  hProcess,
		IN  DWORD                   dwAddr,
		OUT PDWORD                  pdwDisplacement,
		OUT PIMAGEHLP_LINE          Line
			);


//*****************************************************************************
//*****************************************************************************
void StackDumpDefaultHandler(const char*line)
{
	DEBUG_LOG((line));
}


//*****************************************************************************
//*****************************************************************************
void StackDump(void (*callback)(const char*))
{
	if (callback == NULL) 
	{
		callback = StackDumpDefaultHandler;
	}

	InitSymbolInfo();

#if defined(_M_IX86)
	DWORD myeip,myesp,myebp;

_asm
{
MYEIP1:
 mov eax, MYEIP1
 mov dword ptr [myeip] , eax
 mov eax, esp
 mov dword ptr [myesp] , eax
 mov eax, ebp
 mov dword ptr [myebp] , eax
}
#else
	DWORD_PTR myeip,myesp,myebp;
	{
		CONTEXT capture;
		RtlCaptureContext(&capture);
		myeip = (DWORD_PTR)capture.Rip;
		myesp = (DWORD_PTR)capture.Rsp;
		myebp = (DWORD_PTR)capture.Rbp;
	}
#endif


	MakeStackTrace(myeip,myesp,myebp, 2, callback);
}


//*****************************************************************************
//*****************************************************************************
void StackDumpFromContext(DWORD_PTR eip,DWORD_PTR esp,DWORD_PTR ebp, void (*callback)(const char*))
{
	if (callback == NULL) 
	{
		callback = StackDumpDefaultHandler;
	}

	InitSymbolInfo();

	MakeStackTrace(eip,esp,ebp, 0,  callback);
}


//*****************************************************************************
//*****************************************************************************
BOOL InitSymbolInfo()
{
	if (gsInit == TRUE) 
		return TRUE;

	gsInit = TRUE;

	atexit(UninitSymbolInfo);

	// See if we have the line from address function
	// We use GetProcAddress to stop link failures at dll loadup
	HINSTANCE hInstDebugHlp = GetModuleHandle("dbghelp.dll");

	gsSymGetLineFromAddr = (BOOL (__stdcall *)(	IN  HANDLE,IN  DWORD,OUT PDWORD,OUT PIMAGEHLP_LINE))
							GetProcAddress(hInstDebugHlp , "SymGetLineFromAddr");

	char pathname[_MAX_PATH+1];
	char drive[10];
	char directory[_MAX_PATH+1];
	HANDLE process;


	::SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_OMAP_FIND_NEAREST);

	process = GetCurrentProcess();

	//Get the apps name
	::GetModuleFileName(NULL, pathname, _MAX_PATH);

	// turn it into a search path
	_splitpath(pathname, drive, directory, NULL, NULL);
	sprintf(pathname, "%s:\\%s", drive, directory);

	// append the current directory to build a search path for SymInit
	::lstrcat(pathname, ";.;");

	if(::SymInitialize(process, pathname, FALSE))
	{
		// regenerate the name of the app
		::GetModuleFileName(NULL, pathname, _MAX_PATH);
		if(::SymLoadModule(process, NULL, pathname, NULL, 0, 0))
		{
				//Load any other relevant modules (ie dlls) here
				return TRUE;
		}
		::SymCleanup(process);
	}

	return(FALSE);
}


//*****************************************************************************
//*****************************************************************************
void UninitSymbolInfo(void)
{
	if (gsInit == FALSE)
	{
		return;
	}

	gsInit = FALSE;

	::SymCleanup(GetCurrentProcess());
}



//*****************************************************************************
//*****************************************************************************
void MakeStackTrace(DWORD_PTR myeip,DWORD_PTR myesp,DWORD_PTR myebp, int skipFrames, void (*callback)(const char*))
{
STACKFRAME      stack_frame;
BOOL            b_ret = TRUE;

HANDLE thread = GetCurrentThread();
HANDLE process = GetCurrentProcess();

memset(&gsContext, 0, sizeof(CONTEXT));
gsContext.ContextFlags = CONTEXT_FULL;

memset(&stack_frame, 0, sizeof(STACKFRAME));
stack_frame.AddrPC.Mode = AddrModeFlat;
stack_frame.AddrPC.Offset = myeip;
stack_frame.AddrStack.Mode = AddrModeFlat;
stack_frame.AddrStack.Offset = myesp;
stack_frame.AddrFrame.Mode = AddrModeFlat;
stack_frame.AddrFrame.Offset = myebp;
{
/*
    if(GetThreadContext(thread, &gsContext))
    {
        memset(&stack_frame, 0, sizeof(STACKFRAME));
        stack_frame.AddrPC.Mode = AddrModeFlat;
        stack_frame.AddrPC.Offset = gsContext.Eip;
        stack_frame.AddrStack.Mode = AddrModeFlat;
        stack_frame.AddrStack.Offset = gsContext.Esp;
        stack_frame.AddrFrame.Mode = AddrModeFlat;
        stack_frame.AddrFrame.Offset = gsContext.Ebp;
*/

		//{
			callback("Call Stack\n**********\n");

			// Skip some ?
			unsigned int skip = skipFrames;
			while (b_ret&&skip)
			{
					b_ret = StackWalk(      EXC_IMAGE_MACHINE,
											process,
											thread,
											&stack_frame,
											NULL, //&gsContext,
											NULL,
											SymFunctionTableAccess,
											SymGetModuleBase,
											NULL);
					skip--;
			}

			skip = 30;
			while(b_ret&&skip)
			{

					b_ret = StackWalk(      EXC_IMAGE_MACHINE,
											process,
											thread,
											&stack_frame,
											NULL, //&gsContext,
											NULL,
											SymFunctionTableAccess,
											SymGetModuleBase,
											NULL);
					

					
					if (b_ret) WriteStackLine((void *)(DWORD_PTR)stack_frame.AddrPC.Offset, callback);
					skip--;
			}
	}
}


//*****************************************************************************
//*****************************************************************************
void GetFunctionDetails(void *pointer, char*name, char*filename, unsigned int* linenumber, unsigned int* address)
{
	InitSymbolInfo();
	if (name)
	{
		strcpy(name, "<Unknown>");
	}
	if (filename)
	{
		strcpy(filename, "<Unknown>");
	}
	if (linenumber)
	{
		*linenumber = 0xFFFFFFFF;
	}
	if (address)
	{
		*address = 0xFFFFFFFF;
	}

	ULONG displacement = 0;

    HANDLE process = ::GetCurrentProcess();

    char symbol_buffer[512 + sizeof(IMAGEHLP_SYMBOL)];
    memset(symbol_buffer, 0, sizeof(symbol_buffer));

    PIMAGEHLP_SYMBOL psymbol = (PIMAGEHLP_SYMBOL)symbol_buffer;
    psymbol->SizeOfStruct = sizeof(symbol_buffer);
    psymbol->MaxNameLength = 512;

    if (SymGetSymFromAddr(process, (DWORD64)(DWORD_PTR)pointer, &displacement, psymbol))
    {
		if (name)
		{
			strcpy(name, psymbol->Name);
			strcat(name, "();");
		}

		// Get line now
		if (gsSymGetLineFromAddr)
		{
			// Unsupported for win95/98 at least with my current dbghelp.dll

			IMAGEHLP_LINE line;
			memset(&line,0,sizeof(line));
			line.SizeOfStruct = sizeof(line);

		
			if (gsSymGetLineFromAddr(process, (DWORD)(DWORD_PTR)pointer, &displacement, &line))
			{
				if (filename)
				{
					strcpy(filename, line.FileName);
				}
				if (linenumber)
				{
					*linenumber = (unsigned int)line.LineNumber;
				}
				if (address)
				{
					*address = (unsigned int)line.Address;
				}
			} 					
		}
    }
}


//*****************************************************************************
// Gets last x addresses from the stack
//*****************************************************************************
void FillStackAddresses(void**addresses, unsigned int count, unsigned int skip)
{
	InitSymbolInfo();

	STACKFRAME	stack_frame;

	
	HANDLE thread = GetCurrentThread();
	HANDLE process = GetCurrentProcess();

    memset(&gsContext, 0, sizeof(CONTEXT));
    gsContext.ContextFlags = CONTEXT_FULL;

#if defined(_M_IX86)
	DWORD myeip,myesp,myebp;
_asm
{
MYEIP2:
 mov eax, MYEIP2
 mov dword ptr [myeip] , eax
 mov eax, esp
 mov dword ptr [myesp] , eax
 mov eax, ebp
 mov dword ptr [myebp] , eax
 xor eax,eax
}
#else
	DWORD_PTR myeip,myesp,myebp;
	{
		CONTEXT capture;
		RtlCaptureContext(&capture);
		myeip = (DWORD_PTR)capture.Rip;
		myesp = (DWORD_PTR)capture.Rsp;
		myebp = (DWORD_PTR)capture.Rbp;
	}
#endif
memset(&stack_frame, 0, sizeof(STACKFRAME));
stack_frame.AddrPC.Mode = AddrModeFlat;
stack_frame.AddrPC.Offset = myeip;
stack_frame.AddrStack.Mode = AddrModeFlat;
stack_frame.AddrStack.Offset = myesp;
stack_frame.AddrFrame.Mode = AddrModeFlat;
stack_frame.AddrFrame.Offset = myebp;

{
/*
    if(GetThreadContext(thread, &gsContext))
    {
        memset(&stack_frame, 0, sizeof(STACKFRAME));
        stack_frame.AddrPC.Mode = AddrModeFlat;
        stack_frame.AddrPC.Offset = gsContext.Eip;
        stack_frame.AddrStack.Mode = AddrModeFlat;
        stack_frame.AddrStack.Offset = gsContext.Esp;
        stack_frame.AddrFrame.Mode = AddrModeFlat;
        stack_frame.AddrFrame.Offset = gsContext.Ebp;
*/

		Bool stillgoing = TRUE;
//	unsigned int cd = count;

		// Skip some?
		while (stillgoing&&skip)
		{
			stillgoing = StackWalk(EXC_IMAGE_MACHINE,
								process,
								thread,
								&stack_frame,
								NULL,	//&gsContext,
								NULL,
								SymFunctionTableAccess,
								SymGetModuleBase,
								NULL) != 0;
			skip--;
		}

		while(stillgoing&&count)
		{
			stillgoing = StackWalk(EXC_IMAGE_MACHINE,
								process,
								thread,
								&stack_frame,
								NULL, //&gsContext,
								NULL,
								SymFunctionTableAccess,
								SymGetModuleBase,
								NULL) != 0;
			if (stillgoing)
			{
				*addresses  = (void*)(DWORD_PTR)stack_frame.AddrPC.Offset;				
				addresses++;
				count--;
			}
		}

		// Fill remainder
		while (count)
		{
			*addresses = NULL;
			addresses++;
			count--;
		}

	}
/*
	else
	{
		memset(addresses,NULL,count*sizeof(void*));
	}
*/	
}



//*****************************************************************************
// Do full stack dump using an address array
//*****************************************************************************
void StackDumpFromAddresses(void**addresses, unsigned int count, void (*callback)(const char *))
{
	if (callback == NULL) 
	{
		callback = StackDumpDefaultHandler;
	}

	InitSymbolInfo();

	while ((count--) && (*addresses!=NULL))
	{
		WriteStackLine(*addresses,callback);	
		addresses++;
	}	
}


AsciiString g_LastErrorDump;
//*****************************************************************************
//*****************************************************************************
void WriteStackLine(void*address, void (*callback)(const char*))
{
	static char line[MAX_PATH];
	static char function_name[512];
	static char filename[MAX_PATH];
	unsigned int linenumber;
	unsigned int addr;

	GetFunctionDetails(address, function_name, filename, &linenumber, &addr);
    sprintf(line, "  %s(%d) : %s 0x%08p", filename, linenumber, function_name, address);
		if (g_LastErrorDump.isNotEmpty()) {
			g_LastErrorDump.concat(line);
			g_LastErrorDump.concat("\n");
		}
	callback(line);
	callback("\n");
}


//*****************************************************************************
//*****************************************************************************
void DumpExceptionInfo( unsigned int u, EXCEPTION_POINTERS* e_info )
{
   DEBUG_LOG(( "\n********** EXCEPTION DUMP ****************\n" ));
	/*
	** List of possible exceptions
	*/
	 g_LastErrorDump.clear(); 

	static const unsigned int _codes[] = {
		EXCEPTION_ACCESS_VIOLATION,
		EXCEPTION_ARRAY_BOUNDS_EXCEEDED,
		EXCEPTION_BREAKPOINT,
		EXCEPTION_DATATYPE_MISALIGNMENT,
		EXCEPTION_FLT_DENORMAL_OPERAND,
		EXCEPTION_FLT_DIVIDE_BY_ZERO,
		EXCEPTION_FLT_INEXACT_RESULT,
		EXCEPTION_FLT_INVALID_OPERATION,
		EXCEPTION_FLT_OVERFLOW,
		EXCEPTION_FLT_STACK_CHECK,
		EXCEPTION_FLT_UNDERFLOW,
		EXCEPTION_ILLEGAL_INSTRUCTION,
		EXCEPTION_IN_PAGE_ERROR,
		EXCEPTION_INT_DIVIDE_BY_ZERO,
		EXCEPTION_INT_OVERFLOW,
		EXCEPTION_INVALID_DISPOSITION,
		EXCEPTION_NONCONTINUABLE_EXCEPTION,
		EXCEPTION_PRIV_INSTRUCTION,
		EXCEPTION_SINGLE_STEP,
		EXCEPTION_STACK_OVERFLOW,
		0xffffffff
	};

	/*
	** Information about each exception type.
	*/
	static char const * _code_txt[] = {
		"Error code: EXCEPTION_ACCESS_VIOLATION\nDescription: The thread tried to read from or write to a virtual address for which it does not have the appropriate access.",
		"Error code: EXCEPTION_ARRAY_BOUNDS_EXCEEDED\nDescription: The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking.",
		"Error code: EXCEPTION_BREAKPOINT\nDescription: A breakpoint was encountered.",
		"Error code: EXCEPTION_DATATYPE_MISALIGNMENT\nDescription: The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on.",
		"Error code: EXCEPTION_FLT_DENORMAL_OPERAND\nDescription: One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value.",
		"Error code: EXCEPTION_FLT_DIVIDE_BY_ZERO\nDescription: The thread tried to divide a floating-point value by a floating-point divisor of zero.",
		"Error code: EXCEPTION_FLT_INEXACT_RESULT\nDescription: The result of a floating-point operation cannot be represented exactly as a decimal fraction.",
		"Error code: EXCEPTION_FLT_INVALID_OPERATION\nDescription: Some strange unknown floating point operation was attempted.",
		"Error code: EXCEPTION_FLT_OVERFLOW\nDescription: The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type.",
		"Error code: EXCEPTION_FLT_STACK_CHECK\nDescription: The stack overflowed or underflowed as the result of a floating-point operation.",
		"Error code: EXCEPTION_FLT_UNDERFLOW\nDescription:	The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type.",
		"Error code: EXCEPTION_ILLEGAL_INSTRUCTION\nDescription:	The thread tried to execute an invalid instruction.",
		"Error code: EXCEPTION_IN_PAGE_ERROR\nDescription:	The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network.",
		"Error code: EXCEPTION_INT_DIVIDE_BY_ZERO\nDescription: The thread tried to divide an integer value by an integer divisor of zero.",
		"Error code: EXCEPTION_INT_OVERFLOW\nDescription: The result of an integer operation caused a carry out of the most significant bit of the result.",
		"Error code: EXCEPTION_INVALID_DISPOSITION\nDescription: An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception.",
		"Error code: EXCEPTION_NONCONTINUABLE_EXCEPTION\nDescription: The thread tried to continue execution after a noncontinuable exception occurred.",
		"Error code: EXCEPTION_PRIV_INSTRUCTION\nDescription: The thread tried to execute an instruction whose operation is not allowed in the current machine mode.",
		"Error code: EXCEPTION_SINGLE_STEP\nDescription: A trace trap or other single-instruction mechanism signaled that one instruction has been executed.",
		"Error code: EXCEPTION_STACK_OVERFLOW\nDescription: The thread used up its stack.",
		"Error code: ?????\nDescription: Unknown exception."
	};

	DEBUG_LOG( ("Dump exception info\n") );
	CONTEXT *context = e_info->ContextRecord;
	/*
	** The following are set for access violation only
	*/
	int access_read_write=-1;
	unsigned long access_address = 0;
	AsciiString msg; 

// DOUBLE_DEBUG does a DEBUG_LOG, and concats to g_LastErrorDump.  jba.
#define DOUBLE_DEBUG(x) { msg.format x; g_LastErrorDump.concat(msg); DEBUG_LOG( x ); }

	if ( e_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
	{
		DOUBLE_DEBUG (("Exception is access violation\n"));
		access_read_write = e_info->ExceptionRecord->ExceptionInformation[0];  // 0=read, 1=write
		access_address = e_info->ExceptionRecord->ExceptionInformation[1];
	}
	else
	{
		DOUBLE_DEBUG (("Exception code is %x\n", e_info->ExceptionRecord->ExceptionCode));
	}
	Int *winMainAddr = (Int *)WinMain;
	DOUBLE_DEBUG(("WinMain at %x\n", winMainAddr));
	/*
	** Match the exception type with the error string and print it out
	*/
	for ( int i=0 ; _codes[i] != 0xffffffff ; i++ )
	{
		if ( _codes[i] == e_info->ExceptionRecord->ExceptionCode )
		{
			DEBUG_LOG ( ("Found exception description\n") );
			break;
		}
	}
	DOUBLE_DEBUG( ("%s\n", _code_txt[i]));
	/** For access violations, print out the violation address and if it was read or write.
	*/
	if ( e_info->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION )
	{
		if ( access_read_write )
		{
			DOUBLE_DEBUG( ("Access address:%08X was written to.\n", access_address));
		}
		else
		{
			DOUBLE_DEBUG( ("Access address:%08X was read from.\n", access_address));
		}
	}

	DOUBLE_DEBUG (("\nStack Dump:\n"));
	StackDumpFromContext((DWORD_PTR)EXC_CTX_IP(context), (DWORD_PTR)EXC_CTX_SP(context), (DWORD_PTR)EXC_CTX_FP(context), NULL);

	DOUBLE_DEBUG (("\nDetails:\n"));

	DOUBLE_DEBUG (("Register dump...\n"));

	/*
	** Dump the registers.
	*/
	DOUBLE_DEBUG ( ( "Eip:%08X\tEsp:%08X\tEbp:%08X\n", (unsigned)EXC_CTX_IP(context), (unsigned)EXC_CTX_SP(context), (unsigned)EXC_CTX_FP(context)));
	DOUBLE_DEBUG ( ( "Eax:%08X\tEbx:%08X\tEcx:%08X\n", (unsigned)EXC_CTX_R0(context), (unsigned)EXC_CTX_R1(context), (unsigned)EXC_CTX_R2(context)));
	DOUBLE_DEBUG ( ( "Edx:%08X\tEsi:%08X\tEdi:%08X\n", (unsigned)EXC_CTX_R3(context), (unsigned)EXC_CTX_R4(context), (unsigned)EXC_CTX_R5(context)));
	DOUBLE_DEBUG ( ( "EFlags:%08X \n", (unsigned)EXC_CTX_FLAGS(context)));
	#if defined(_M_IX86)
	DOUBLE_DEBUG ( ( "CS:%04x  SS:%04x  DS:%04x  ES:%04x  FS:%04x  GS:%04x\n", context->SegCs, context->SegSs, context->SegDs, context->SegEs, context->SegFs, context->SegGs));
#else
	DOUBLE_DEBUG ( ( "DS:%04x  ES:%04x  FS:%04x  GS:%04x\n", context->SegDs, context->SegEs, context->SegFs, context->SegGs));
#endif

	/*
	** Dump the bytes at EIP. This will make it easier to match the crash address with later versions of the game.
	*/
	char scrap[512];
	DOUBLE_DEBUG ( ("EIP bytes dump...\n"));
	wsprintf (scrap, "\nBytes at CS:EIP (%08X)  : ", (unsigned)EXC_CTX_IP(context));

	unsigned char *eip_ptr = (unsigned char *)(DWORD_PTR)EXC_CTX_IP(context);
	char bytestr[32];

	for (int c = 0 ; c < 32 ; c++)
	{
		if (IsBadReadPtr(eip_ptr, 1))
		{
			lstrcat (scrap, "?? ");
		}
		else
		{
			sprintf (bytestr, "%02X ", *eip_ptr);
			strcat (scrap, bytestr);
		}
		eip_ptr++;
	}

	strcat (scrap, "\n");
	DOUBLE_DEBUG ( ( (scrap)));
  DEBUG_LOG(( "********** END EXCEPTION DUMP ****************\n\n" ));
}																									 


#pragma pack(pop)

#endif

