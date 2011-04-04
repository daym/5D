// stdafx.cpp : source file that includes just the standard includes
// 4D.pch will be the pre-compiled header
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
	str = (FILE*) calloc(1, sizeof(FILE));
#endif
	str->_flag = _IOREAD|_IOSTRG|_IOMYBUF;
	str->_ptr = str->_base = (char *)contents;
	str->_cnt = contents_length; // INT_MAX;
	return(str);
	/*
	str->_ptr = self->_base = (char*) contents;
	str->_cnt;
	char*	_base;
	int	_flag;
	int	_file;
	int	_charbuf;
	int	_bufsiz;
	char*	_tmpfname;*/

}
