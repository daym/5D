// stdafx.cpp : source file that includes just the standard includes
// 5D.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"
//#include <internal.h>

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

extern "C" {
	FILE * __cdecl _getstream(void);
};

FILE* fmemopen(void* contents, size_t contents_length, const char* mode) {
	FILE* str;
#ifdef _MSC_VER
	/*
	InitializeCriticalSection( &(((_FILEX *)str)->lock) );
	__crtInitCritSecAndSpinCount
	*/
	str = _getstream(); // (FILE*) calloc(1, sizeof(FILE));
#else
	str = _tfdopen(0, _T("r"));
	//str = (FILE*) calloc(1, sizeof(FILE));
	str->_bufsiz = 0;
#endif
	str->_flag = _IOREAD|_IOSTRG|_IOMYBUF;
	str->_ptr = str->_base = (char *)contents;
	str->_cnt = contents_length; // INT_MAX;
	return(str);
	/*
	int	_file;
	int	_charbuf;
	char*	_tmpfname;*/

}

char* ToUTF8(const std::wstring& source) {
	int count = WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, NULL, 0, NULL, NULL);
	char* result = (char*) calloc(count, sizeof(char));
	if(WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, result, count, NULL, NULL) != count)
		abort();
	return(result);
}

std::wstring FromUTF8(const char* source) {
	int count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
	WCHAR* result = (WCHAR*) calloc(count, sizeof(WCHAR));
	if(MultiByteToWideChar(CP_UTF8, 0, source, -1, result, count) != count)
		abort();
	return(result);
}
std::wstring GetWIN32Diagnostics(void) {
	LPVOID lpMsgBuf;
	//LPVOID lpDisplayBuf;
	std::wstring result;
	DWORD dw = GetLastError(); 
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL) > 0) {
		result = (LPWSTR) lpMsgBuf;
		LocalFree(lpMsgBuf);
	} else
		result = _T("???");
	return(result);
}
