/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef WIN32
#include "stdafx.h"
#else
#include <dirent.h>
#include <unistd.h>
#endif
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Numbers/Integer"
#include "Numbers/Real"
#include "Evaluators/Operation"
#include "FFIs/Allocators"
#include "Numbers/Ratio"

namespace Evaluators {
using namespace AST;
using namespace Numbers;

typedef long long long_long;
typedef long double long_double;

#define IMPLEMENT_NATIVE_INT_GETTER(typ) \
typ get_##typ(AST::NodeT root) { \
	NativeInt result2 = 0; \
	typ result = 0; \
	if(!Numbers::toNativeInt(root, result2) || (result = result2) != result2) \
		throw Evaluators::EvaluationException("value out of range for " #typ); \
	return(result); \
}
// TODO support Ratio - at least for the floats, maybe.
#define IMPLEMENT_NATIVE_FLOAT_GETTER(typ) \
typ get_##typ(AST::NodeT root) { \
	NativeFloat result2 = 0.0; \
	typ result = 0; \
	if(Numbers::ratio_P(root)) \
		root = Evaluators::divideA(Ratio_getA(root), Ratio_getB(root), NULL); \
	if(!Numbers::toNativeFloat(root, result2) || (result = result2) != result2) \
		throw Evaluators::EvaluationException("value out of range for " #typ); \
	return(result); \
}
int get_nearest_int(AST::NodeT root) {
	NativeInt result2 = 0;
	int result = 0;
	if(!Numbers::toNearestNativeInt(root, result2))
		throw Evaluators::EvaluationException("cannot convert to int");
	if((result = result2) != result2) { /* doesn't fit */
		return (result2 < 0) ? INT_MIN : INT_MAX;
	}
	return(result);
}

IMPLEMENT_NATIVE_INT_GETTER(int)
IMPLEMENT_NATIVE_INT_GETTER(long)
IMPLEMENT_NATIVE_INT_GETTER(long_long)
IMPLEMENT_NATIVE_INT_GETTER(short)
IMPLEMENT_NATIVE_FLOAT_GETTER(float)
IMPLEMENT_NATIVE_FLOAT_GETTER(double)
IMPLEMENT_NATIVE_FLOAT_GETTER(long_double)

void* get_pointer(AST::NodeT root) {
	Box* rootBox = dynamic_cast<Box*>(root);
	if(rootBox)
		return(rootBox->native);
	else {
		std::stringstream sst;
		sst << "cannot get native pointer for " << str(root);
		std::string v = sst.str();
		throw Evaluators::EvaluationException(v.c_str());
	}
}
static Int int01(1);
static Int int00(0);
bool get_boolean(AST::NodeT root) {
	AST::NodeT result = Evaluators::reduce(AST::makeApplication(AST::makeApplication(root, &int01), &int00));
	if(result == NULL)
		throw Evaluators::EvaluationException("that cannot be reduced to a boolean");
	return(result == &int01);
}
char* get_string(AST::NodeT root) {
	AST::Str* rootString = dynamic_cast<AST::Str*>(root);
	if(rootString) {
		// TODO maybe check terminating zero? Maybe not.
		return((char*) rootString->native);
	} else {
		std::stringstream sst;
		sst << "cannot get native string for " << str(root);
		std::string v = sst.str();
		throw Evaluators::EvaluationException(v.c_str());
		//AST::Str* v = AST::makeStrCXX(str(root));
		//return((char*) v->native);
	}
}
size_t get_string_length(AST::NodeT root) {
	AST::Str* rootString = dynamic_cast<AST::Str*>(root);
	if(rootString) {
		return(rootString->size);
	} else {
		std::stringstream sst;
		sst << "cannot get string length of " << str(root);
		std::string v = sst.str();
		throw Evaluators::EvaluationException(v.c_str());
		//AST::Str* v = AST::makeStrCXX(str(root));
		//return((char*) v->native);
	}
}
static AST::NodeT wrapWrite(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	FILE* f = (FILE*) Evaluators::get_pointer(iter->second);
	++iter;
	char* text = iter->second ? get_string(iter->second) : NULL;
	++iter;
	AST::NodeT world = iter->second;
	size_t result = fwrite(text ? text : "", 1, text ? strlen(text) : 0, f);
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
static AST::NodeT wrapFlush(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	FILE* f = (FILE*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	size_t result = fflush(f);
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
// FIXME unlimited length
static AST::NodeT wrapReadLine(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	FILE* f = (FILE*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	char text[2049];
	AST::NodeT result = NULL;
	if(fgets(text, 2048, f)) {
		result = Evaluators::internNative(text);
	} else
		result = NULL;
	// TODO ferror
	return(Evaluators::makeIOMonad(result, world));
}

DEFINE_FULL_OPERATION(Writer, {
	return(wrapWrite(fn, argument));
})
DEFINE_FULL_OPERATION(LineReader, {
	return(wrapReadLine(fn, argument));
})
DEFINE_FULL_OPERATION(Flusher, {
	return(wrapFlush(fn, argument));
})
DEFINE_SIMPLE_OPERATION(ErrnoGetter, Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) errno), reduce(argument)))
#ifdef WIN32
static AST::NodeT wrapMktime(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	struct tm* f = (struct tm*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		// note that WIN32 CHANGES the structure contents!!
		// TODO error check
#ifdef _MSC_VER
		AST::NodeT result = Numbers::internNativeU(_mktime64(f));
#else
		AST::NodeT result = Numbers::internNativeU((unsigned long long) mktime(f));
#endif
		return(Evaluators::makeIOMonad(result, world));
	}
}
/* mkgmtime.c - make time corresponding to a GMT timeval struct
 $Id: mkgmtime.c,v 1.2 2000/12/08 03:25:39 jackson Exp $
 
 * Copyright (c) 1998-2000 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any other legal
 *    details, please contact  
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 *
 */
/*
 * Copyright (c) 1987, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
** Adapted from code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago. (mtxinu!kridle now).
**	It does a binary search of the time_t space.  Since time_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
*/

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

static inline int tmcomp(const struct tm * const atmp, const struct tm * const btmp) {
	register int	result;
	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
		(result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
		(result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
		(result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
		(result = (atmp->tm_min - btmp->tm_min)) == 0)
			result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

time_t emulatemkgmtime(struct tm * const	tmp) {
	register int			dir;
	register int			bits;
	register int			saved_seconds;
	time_t				t;
	struct tm			yourtm, *mytm;

	yourtm = *tmp;
	saved_seconds = yourtm.tm_sec;
	yourtm.tm_sec = 0;
	/*
	** Calculate the number of magnitude bits in a time_t
	** (this works regardless of whether time_t is
	** signed or unsigned, though lint complains if unsigned).
	*/
	for (bits = 0, t = 1; t > 0; ++bits, t <<= 1)
		;
	/*
	** If time_t is signed, then 0 is the median value,
	** if time_t is unsigned, then 1 << bits is median.
	*/
	t = (t < 0) ? 0 : ((time_t) 1 << bits);
	for ( ; ; ) {
		mytm = gmtime(&t);
		dir = tmcomp(mytm, &yourtm);
		if (dir != 0) {
			if (bits-- < 0)
				return WRONG;
			if (bits < 0)
				--t;
			else if (dir > 0)
				t -= (time_t) 1 << bits;
			else	t += (time_t) 1 << bits;
			continue;
		}
		break;
	}
	t += saved_seconds;
	return t;
}
static AST::NodeT wrapMkgmtime(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	struct tm* f = (struct tm*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		// note that WIN32 CHANGES the structure contents!!
		// TODO error check
#ifdef _MSC_VER
		AST::NodeT result = Numbers::internNativeU(_mkgmtime64(f));
#else
		AST::NodeT result = Numbers::internNativeU((unsigned long long) emulatemkgmtime(f));
#endif
		return(Evaluators::makeIOMonad(result, world));
	}
}
#else
static AST::NodeT wrapMktime(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	struct tm* f = (struct tm*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		// TODO error check
		AST::NodeT result = Numbers::internNativeU((unsigned long long) mktime(f));
		return(Evaluators::makeIOMonad(result, world));
	}
}
static AST::NodeT wrapMkgmtime(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	struct tm* f = (struct tm*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		// note that WIN32 CHANGES the structure contents!!
		// TODO error check
		AST::NodeT result = Numbers::internNativeU((unsigned long long) timegm(f));
		return(Evaluators::makeIOMonad(result, world));
	}
}
#endif
DEFINE_FULL_OPERATION(GmtimeMaker, {
	return(wrapMkgmtime(fn, argument));
})
DEFINE_FULL_OPERATION(TimeMaker, {
	return(wrapMktime(fn, argument));
})
REGISTER_BUILTIN(Writer, 3, 0, AST::symbolFromStr("write!"))
REGISTER_BUILTIN(Flusher, 2, 0, AST::symbolFromStr("flush!"))
REGISTER_BUILTIN(LineReader, 2, 0, AST::symbolFromStr("readline!"))
REGISTER_BUILTIN(ErrnoGetter, 1, 0, AST::symbolFromStr("errno!"))
REGISTER_BUILTIN(GmtimeMaker, 2, 0, AST::symbolFromStr("mkgmtime!"))
REGISTER_BUILTIN(TimeMaker, 2, 0, AST::symbolFromStr("mktime!"))
#ifdef WIN32

static AST::NodeT wrapGetWin32FindDataWSize(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	//DIR* f = (DIR*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		AST::NodeT result = Numbers::internNativeU((unsigned long long) sizeof(WIN32_FIND_DATAW));
		return(Evaluators::makeIOMonad(result, world));
	}
}
static AST::NodeT wrapUnpackWin32FindDataW(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	WIN32_FIND_DATAW* f = iter->second ? (WIN32_FIND_DATAW*) Evaluators::get_pointer(iter->second) : NULL;
	++iter;
	AST::NodeT world = iter->second;
	{
	/*typedef struct _WIN32_FIND_DATA {
  DWORD    dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  DWORD    nFileSizeHigh;
  DWORD    nFileSizeLow;
  DWORD    dwReserved0;
  DWORD    dwReserved1;
  TCHAR    cFileName[MAX_PATH];
  TCHAR    cAlternateFileName[14];
} WIN32_FIND_DATA, *PWIN32_FIND_DATA, *LPWIN32_FIND_DATA;*/
		AST::NodeT result = f ? AST::makeCons(AST::makeStr(ToUTF8(f->cFileName)), 
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->dwReserved0),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->dwReserved1),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) 0U),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->dwFileAttributes), NULL))))) : NULL;
		return(Evaluators::makeIOMonad(result, world));
	}
}

DEFINE_FULL_OPERATION(Win32FindDataWSizeGetter, {
	return(wrapGetWin32FindDataWSize(fn, argument));
})
REGISTER_BUILTIN(Win32FindDataWSizeGetter, 2, 0, AST::symbolFromStr("getWin32FindDataWSize!"))
DEFINE_FULL_OPERATION(Win32FindDataWUnpacker, {
	return(wrapUnpackWin32FindDataW(fn, argument));
})
REGISTER_BUILTIN(Win32FindDataWUnpacker, 2, 0, AST::symbolFromStr("unpackWin32FindDataW!"))

#else

static AST::NodeT wrapGetDirentSize(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	DIR* f = (DIR*) Evaluators::get_pointer(iter->second);
	++iter;
	AST::NodeT world = iter->second;
	{
		long namemax;
		namemax = fpathconf(dirfd(f), _PC_NAME_MAX);
		if(namemax == -1)
			; /* TODO error check */
		AST::NodeT result = Numbers::internNativeU((unsigned long long) offsetof(struct dirent, d_name) + (size_t) namemax);
		return(Evaluators::makeIOMonad(result, world));
	}
}
static AST::NodeT wrapUnpackDirent(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	struct dirent* f = iter->second ? (struct dirent*) Evaluators::get_pointer(iter->second) : NULL;
	++iter;
	AST::NodeT world = iter->second;
	{
		AST::NodeT result = f ? AST::makeCons(AST::makeStr(f->d_name), 
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->d_ino),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->d_off),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->d_reclen),
		                    AST::makeCons(Numbers::internNativeU((unsigned long long) f->d_type), NULL))))) : NULL;
		return(Evaluators::makeIOMonad(result, world));
	}
}

DEFINE_FULL_OPERATION(DirentSizeGetter, {
	return(wrapGetDirentSize(fn, argument));
})
REGISTER_BUILTIN(DirentSizeGetter, 2, 0, AST::symbolFromStr("getDirentSize!"))
DEFINE_FULL_OPERATION(DirentUnpacker, {
	return(wrapUnpackDirent(fn, argument));
})
REGISTER_BUILTIN(DirentUnpacker, 2, 0, AST::symbolFromStr("unpackDirent!"))
#endif
}; /* end namespace */
