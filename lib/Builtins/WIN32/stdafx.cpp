// stdafx.cpp : source file that includes just the standard includes
// 5D.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
#include <gc.h>
//#include <internal.h>

std::wstring getWIN32Diagnostics(void) {
	LPVOID lpMsgBuf;
	//LPVOID lpDisplayBuf;
	std::wstring result;
	DWORD dw = GetLastError(); 
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL) > 0) {
		result = (LPWSTR) lpMsgBuf;
		LocalFree(lpMsgBuf);
	}
	return(result);
}
