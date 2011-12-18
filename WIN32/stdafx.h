// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
//#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <string>

#ifdef _MSC_VER
typedef DWORD uint32_t; /* FIXME */
#endif
#define FM_NOTIFY_SYSTEM (WM_USER + 42)

// TODO: reference additional headers your program requires here
FILE* fmemopen(void* contents, size_t contents_length, const char* mode);
char* ToUTF8(const std::wstring& source);
std::wstring FromUTF8(const char* source);
std::wstring GetWIN32Diagnostics(void);
