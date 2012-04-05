/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifdef WIN32
#include "stdafx.h"
#endif
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
REGISTER_BUILTIN(Writer, 3, 0, AST::symbolFromStr("write!"))
REGISTER_BUILTIN(Flusher, 2, 0, AST::symbolFromStr("flush!"))
REGISTER_BUILTIN(LineReader, 2, 0, AST::symbolFromStr("readline!"))
REGISTER_BUILTIN(ErrnoGetter, 1, 0, AST::symbolFromStr("errno!"))

}; /* end namespace */
