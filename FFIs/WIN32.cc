/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <map>
#include "AST/Symbol"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "Evaluators/Operation"
#include "FFIs/FFIs"
#include "AST/Symbols"

namespace FFIs {
using namespace Evaluators;

AST::Node* wrapAccessLibrary(AST::Node* options, AST::Node* argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	Evaluators::CXXArguments::const_iterator endIter = arguments.end();
	assert(iter != endIter);
	assert(iter->first == NULL);
	void* body = Evaluators::get_native_pointer(iter->second);
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::Str* libName = dynamic_cast<AST::Str*>(iter->second);
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::Symbol* signature = dynamic_cast<AST::Symbol*>(iter->second);
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::Node* fnName = iter->second;
	++iter;

	void* nativeProc = body && fnName ? (void*) GetProcAddress((HMODULE) body, Evaluators::get_native_string(fnName)) : NULL; // FIXME
	// filename is the second argument, so ignore.
	//return(Evaluators::reduce(AST::makeApplication(body, argument)));
	return(new CProcedure(nativeProc, AST::makeApplication(AST::makeApplication(AST::makeApplication(AST::symbolFromStr("requireSharedLibrary"), libName), quote(signature)), fnName), strlen(signature->name) - 2 + 1/*monad*/, 0, signature));
}
AST::Node* wrapLoadLibraryC(const char* name) {
	std::wstring nameW = FromUTF8(name);
	void* clib = LoadLibraryW(nameW.c_str());
	if(!clib) {
		std::wstring err = GetWIN32Diagnostics();
		fprintf(stderr, "(dlopen \"%s\") failed because: %s\n", name, err.c_str());
	}
	return(AST::makeBox(clib));
	//return(AST::makeAbstraction(AST::symbolFromStr("name"), result));
}
static AST::Node* wrapLoadLibrary(AST::Node* options, AST::Node* filename) {
	filename = reduce(filename);
	//std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, filename);
	// struct REPL* self = dynamic_cast<struct REPL*>(arguments.front().second);
	//assert(self);
	AST::Node* body = wrapLoadLibraryC(Evaluators::get_native_string(filename));
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&SharedLibrary, body)), filename)));
}

DEFINE_FULL_OPERATION(SharedLibraryLoader, {
	return(wrapLoadLibrary(fn, argument));
})
DEFINE_FULL_OPERATION(SharedLibrary, {
	return(wrapAccessLibrary(fn, argument));
})
REGISTER_BUILTIN(SharedLibraryLoader, 1, 0, AST::symbolFromStr("requireSharedLibrary"));
REGISTER_BUILTIN(SharedLibrary, 4, 1, AST::symbolFromStr("requireSharedLibrary"));

};
