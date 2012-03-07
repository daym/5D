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
#ifndef WIN32
#include <sys/utsname.h>
#endif
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Numbers/Integer"
#include "Numbers/Real"
#include "Evaluators/Operation"
#include "FFIs/Allocators"

namespace Evaluators {
using namespace AST;
using namespace Numbers;

typedef long long long_long;
typedef long double long_double;

#define IMPLEMENT_NATIVE_INT_GETTER(typ) \
typ get_##typ(AST::Node* root) { \
	NativeInt result2 = 0; \
	typ result = 0; \
	if(!Numbers::toNativeInt(root, result2) || (result = result2) != result2) \
		throw Evaluators::EvaluationException("value out of range for " #typ); \
	return(result); \
}
#define IMPLEMENT_NATIVE_FLOAT_GETTER(typ) \
typ get_##typ(AST::Node* root) { \
	NativeFloat result2 = 0.0; \
	typ result = 0; \
	if(!Numbers::toNativeFloat(root, result2) || (result = result2) != result2) \
		throw Evaluators::EvaluationException("value out of range for " #typ); \
	return(result); \
}
int get_nearest_int(AST::Node* root) {
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

void* get_pointer(AST::Node* root) {
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
bool get_boolean(AST::Node* root) {
	/* FIXME */
	return(false);
}
char* get_string(AST::Node* root) {
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

static AST::Node* wrapWrite(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	char* text = iter->second ? get_string(iter->second) : NULL;
	++iter;
	if(!f)
		throw Evaluators::EvaluationException("need file, but is missing");
	AST::Node* world = iter->second;
	size_t result = fwrite(text ? text : "", 1, text ? strlen(text) : 0, (FILE*) f->native);
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
static AST::Node* wrapFlush(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	if(!f)
		throw Evaluators::EvaluationException("need file, but is missing");
	size_t result = fflush((FILE*) Evaluators::get_pointer(f));
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
// FIXME unlimited length
static AST::Node* wrapReadLine(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	char text[2049];
	AST::Node* result = NULL;
	if(!f)
		throw Evaluators::EvaluationException("need file, but is missing");
	if(fgets(text, 2048, (FILE*) f->native)) {
		result = Evaluators::internNative(text);
	} else
		result = NULL;
	// TODO ferror
	return(Evaluators::makeIOMonad(result, world));
}

static AST::Node* wrapGetAbsolutePath(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	char* text = iter->second ? get_string(iter->second) : NULL;
	++iter;
	AST::Node* world = iter->second;
	text = get_absolute_path(text);
	return(Evaluators::makeIOMonad(AST::makeStr(text), world));
}
static AST::Node* internEnviron(const char** envp) {
	if(*envp) {
		AST::Node* head = AST::makeStr(*envp++);
		return(AST::makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static AST::Node* wrapInternEnviron(AST::Node* argument) {
	AST::Box* envp = dynamic_cast<AST::Box*>(argument);
	// TODO check whether it worked? No.
	return internEnviron((const char**) envp->native);
}
static AST::Box* environFromList(AST::Node* argument) {
	int count = 0;
	char** result;
	int i = 0;
	AST::Node* listNode = reduce(argument);
	for(AST::Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		++count;
		// FIXME handle overflow
	}
	result = (char**) GC_MALLOC(sizeof(char*) * (count + 1));
	for(AST::Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		result[i] = Evaluators::get_string(node->head);
		++i;
	}
	return(AST::makeBox(result, AST::makeApplication(&EnvironFromList, listNode/* or argument*/)));
}
DEFINE_SIMPLE_OPERATION(EnvironInterner, wrapInternEnviron(reduce(argument)))
DEFINE_SIMPLE_OPERATION(EnvironFromList, environFromList(argument))
DEFINE_FULL_OPERATION(Writer, {
	return(wrapWrite(fn, argument));
})
DEFINE_FULL_OPERATION(LineReader, {
	return(wrapReadLine(fn, argument));
})
DEFINE_FULL_OPERATION(Flusher, {
	return(wrapFlush(fn, argument));
})
DEFINE_FULL_OPERATION(AbsolutePathGetter, {
	return(wrapGetAbsolutePath(fn, argument));
})
#ifdef WIN32
static AST::Str* get_arch_dep_path(AST::Str* nameNode) {
	return(nameNode);
}
bool absolute_path_P(AST::Str* name) {
	if(name == NULL) // an empty path is not an absolute path.
		return(false);
	if(name->size < 2)
		return(false);
	char* c = (char*) name->native;
	if(*c != '\\' && *c != '/' && *(c + 1) != ':')
		return(false);
	if(*c == '\\' || *c == '/')
		return(true);
	++c;
	if(*c == ':')
		return(true);
	return(false);
}
#else
static AST::Str* get_arch_dep_path(AST::Str* nameNode) {
	if(nameNode == NULL)
		return(NULL);
	// keep that result constant and invariant.
	std::stringstream sst;
	std::string name((char*) nameNode->native, nameNode->size);
	sst << "/lib/";
	struct utsname buf;
	if(uname(&buf) == -1)
		sst << "x86_64";
	else
		sst << buf.machine;
	sst << "-linux-gnu/";
	sst << name;
	return(AST::makeStrCXX(sst.str()));
}
bool absolute_path_P(AST::Str* name) {
	if(name == NULL) // an empty path is not an absolute path.
		return(false);
	if(name->size < 1)
		return(false);
	char* c = (char*) name->native;
	return(*c == '/');
}
#endif

DEFINE_SIMPLE_OPERATION(ArchDepLibNameGetter, get_arch_dep_path(dynamic_cast<AST::Str*>(reduce(argument))))
DEFINE_SIMPLE_OPERATION(AbsolutePathP, absolute_path_P(dynamic_cast<AST::Str*>(reduce(argument))))
DEFINE_SIMPLE_OPERATION(ErrnoGetter, Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) errno), reduce(argument)))
DEFINE_SIMPLE_OPERATION(EnvironGetter, Evaluators::makeIOMonad(internEnviron((const char**) environ), reduce(argument)))
REGISTER_BUILTIN(Writer, 3, 0, AST::symbolFromStr("write!"))
REGISTER_BUILTIN(Flusher, 2, 0, AST::symbolFromStr("flush!"))
REGISTER_BUILTIN(LineReader, 2, 0, AST::symbolFromStr("readline!"))
REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, AST::symbolFromStr("absolutePath!"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, AST::symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, AST::symbolFromStr("absolutePath?"))
REGISTER_BUILTIN(ErrnoGetter, 1, 0, AST::symbolFromStr("errno!"))
REGISTER_BUILTIN(EnvironGetter, 1, 0, AST::symbolFromStr("environ!"))
REGISTER_BUILTIN(EnvironInterner, 1, 0, AST::symbolFromStr("listFromEnviron"))
REGISTER_BUILTIN(EnvironFromList, 1, 0, AST::symbolFromStr("environFromList"))

char* get_absolute_path(const char* filename) {
#ifdef WIN32
	if(filename == NULL || filename[0] == 0)
		filename = ".";
	std::wstring filenameW = FromUTF8(filename);
	WCHAR buffer[2049];
	if(GetFullPathNameW(filenameW.c_str(), 2048, buffer, NULL) != 0) {
		return(ToUTF8(buffer));
	} else
		return(GCx_strdup(filename));
#else
	if(filename && filename[0] == '/')
		return(GCx_strdup(filename));
	else {
		char buffer[2049];
		std::stringstream sst;
		if(getcwd(buffer, 2048)) {
			sst << buffer;
			if(buffer[0] && buffer[strlen(buffer) - 1] != '/')
				sst << '/';
		}
		if(filename)
			sst << filename;
		std::string v = sst.str();
		return(GCx_strdup(v.c_str()));
	}
#endif
}


}; /* end namespace */
