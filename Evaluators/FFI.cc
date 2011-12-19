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
#ifndef WIN32
#include <sys/utsname.h>
#endif
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Numbers/Integer"
#include "Numbers/Real"
#include "Evaluators/Operation"

namespace Evaluators {
using namespace AST;
using namespace Numbers;

typedef long long long_long;

#define IMPLEMENT_NATIVE_GETTER(typ) \
typ get_native_##typ(AST::Node* root) { \
	bool B_ok; \
	typ result = Numbers::toNativeInt(root, B_ok); \
	if(!B_ok) \
		result = 0; /* FIXME FALLBACK */ \
	return(result); \
	/* FIXME support bigger than NativeInt */ \
}

float get_native_float(AST::Node* root) {
	bool B_ok;
	float result = Numbers::toNativeFloat(root, B_ok);
	if(!B_ok) {
		std::stringstream sst;
		sst << "cannot get native float for " << str(root);
		std::string v = sst.str();
		throw Evaluators::EvaluationException(v.c_str());
	}
	return(result);
	/* FIXME support bigger than NativeInt */
}
long double get_native_long_double(AST::Node* root) {
	// FIXME more precision.
	return(get_native_float(root));
}
double get_native_double(AST::Node* root) {
	// FIXME more precision.
	return(get_native_float(root));
}

IMPLEMENT_NATIVE_GETTER(int)
IMPLEMENT_NATIVE_GETTER(long)
IMPLEMENT_NATIVE_GETTER(long_long)
IMPLEMENT_NATIVE_GETTER(short)

void* get_native_pointer(AST::Node* root) {
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
bool get_native_boolean(AST::Node* root) {
	/* FIXME */
	return(false);
}
char* get_native_string(AST::Node* root) {
	AST::Str* rootString = dynamic_cast<AST::Str*>(root);
	if(rootString) {
		// TODO maybe check terminating zero? Maybe not.
		return((char*) rootString->native);
	} else {
		AST::Str* v = AST::makeStrCXX(str(root));
		return((char*) v->native);
	}
}

// FIXME move these to their own module:
static AST::Node* wrapWrite(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	char* text = get_native_string(iter->second);
	++iter;
	AST::Node* world = iter->second;
	size_t result = fwrite(text, 1, strlen(text), (FILE*) f->native);
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
// FIXME move these to their own module:
static AST::Node* wrapFlush(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	size_t result = fflush((FILE*) Evaluators::get_native_pointer(f));
	return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
// FIXME unlimited length
static AST::Node* wrapReadLine(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	AST::Box* f = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::Node* world = iter->second;
	char text[2049];
	AST::Node* result = NULL;
	if(fgets(text, 2048, (FILE*) f->native)) {
		result = Evaluators::internNative(text);
	} else
		result = NULL;
	// TODO ferror
	return(Evaluators::makeIOMonad(result, world));
}

static AST::Node* wrapGetAbsolutePath(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
	std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
	char* text = iter->second ? get_native_string(iter->second) : NULL;
	++iter;
	AST::Node* world = iter->second;
	text = get_absolute_path(text);
	return(Evaluators::makeIOMonad(AST::makeStr(text), world));
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
REGISTER_BUILTIN(Writer, 3, 0, AST::symbolFromStr("write"))
REGISTER_BUILTIN(Flusher, 2, 0, AST::symbolFromStr("flush"))
REGISTER_BUILTIN(LineReader, 2, 0, AST::symbolFromStr("readline"))
REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, AST::symbolFromStr("absolutePath"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, AST::symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, AST::symbolFromStr("absolutePath?"))

char* get_absolute_path(const char* filename) {
#ifdef WIN32
	if(filename == NULL || filename[0] == 0)
		filename = ".";
	std::wstring filenameW = FromUTF8(filename);
	WCHAR buffer[2049];
	if(GetFullPathNameW(filenameW.c_str(), 2048, buffer, NULL) != 0) {
		return(ToUTF8(buffer));
	} else
		return(strdup(filename));
#else
	if(filename[0] == '/')
		return(strdup(filename));
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
		return(strdup(v.c_str()));
	}
#endif
}


}; /* end namespace */
