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
#include "Values/Values"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include <5D/Operations>
#include <5D/FFIs>
#include "FFIs/FFIs"
#include <5D/WIN32Strings>

namespace FFIs {
using namespace Evaluators;
using namespace Values;

NodeT wrapAccessLibrary(NodeT options, NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	Evaluators::CXXArguments::const_iterator endIter = arguments.end();
	assert(iter != endIter);
	assert(iter->first == NULL);
	void* body = Values::pointerFromNode(iter->second);
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	NodeT libName = iter->second; // Str
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	NodeT signature = iter->second; // Symbol
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	NodeT fnName = iter->second;
	++iter;

	void* nativeProc = body && fnName ? (void*) GetProcAddress((HMODULE) body, Values::stringFromNode(fnName)) : NULL; // FIXME
	// filename is the second argument, so ignore.
	//return(Evaluators::reduce(makeApplication(body, argument)));
	return(new CProcedure(nativeProc, makeApplication(makeApplication(makeApplication(symbolFromStr("requireSharedLibrary"), libName), quote(signature)), fnName), strlen(get_symbol_name(signature)) - 2 + 1/*monad*/, 0, signature));
}
NodeT wrapLoadLibraryC(NodeT nameS) {
	const char* name = Values::stringFromNode(nameS);
	std::wstring nameW = wstringFromUtf8(name);
	void* clib = LoadLibraryW(nameW.c_str());
	if(!clib) {
		std::wstring err = getWIN32Diagnostics();
		fprintf(stderr, "(dlopen \"%s\") failed because: %s\n", name, err.c_str());
	}
	return(makeBox(clib, makeApplication(&SharedLibraryLoader, nameS)));
	//return(makeAbstraction(symbolFromStr("name"), result));
}
static NodeT wrapLoadLibrary(NodeT options, NodeT filename) {
	filename = reduce(filename);
	NodeT body = wrapLoadLibraryC(filename);
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&SharedLibrary, body)), filename)));
}

DEFINE_FULL_OPERATION(SharedLibraryLoader, {
	return(wrapLoadLibrary(fn, argument));
})
DEFINE_FULL_OPERATION(SharedLibrary, {
	return(wrapAccessLibrary(fn, argument));
})
REGISTER_BUILTIN(SharedLibraryLoader, 1, 0, symbolFromStr("requireSharedLibrary"));
REGISTER_BUILTIN(SharedLibrary, 4, 1, symbolFromStr("requireSharedLibrary"));
bool sharedLibraryFileP(const char* name) {
	FILE* input_file;
	char buf[3];
	input_file = fopen(name, "rb");
	if(!input_file)
		return(false);
	if(fread(buf, 3, 1, input_file) != 1) {
		fclose(input_file);
		return(false);
	}
	fclose(input_file);
	return(strncmp(buf, "MZ\220", 3) == 0);
}
        
};
