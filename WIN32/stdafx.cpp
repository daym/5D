// stdafx.cpp : source file that includes just the standard includes
// 4D.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

FILE* fmemopen(void* contents, size_t contents_length, const char* mode) {
	FILE* str;
	str = (FILE*) calloc(1, sizeof(FILE));
	str->_flag = _IOREAD|_IOSTRG|_IOMYBUF;
	str->_ptr = str->_base = (char *)contents;
	str->_cnt = contents_length; // INT_MAX;
	return(str);
}
