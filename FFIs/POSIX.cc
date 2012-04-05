/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <sstream>
#include <map>
#include <dlfcn.h>
#include "AST/Symbol"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "Evaluators/Operation"
#include "FFIs/FFIs"
#include "AST/Symbols"

/* forgotten prototype in gc_pthread_redirects.h */
extern "C" void * GC_dlopen(const char *path, int mode);

namespace FFIs {
using namespace Evaluators;

AST::NodeT wrapAccessLibrary(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	Evaluators::CXXArguments::const_iterator endIter = arguments.end();
	assert(iter != endIter);
	assert(iter->first == NULL);
	void* body = Evaluators::get_pointer(iter->second);
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::NodeT libName = iter->second; // Str
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::NodeT signature = iter->second; // Symbol
	++iter;
	assert(iter != endIter);
	assert(iter->first == NULL);
	AST::NodeT fnName = iter->second;
	++iter;

	void* nativeProc = body && fnName ? dlsym(body, Evaluators::get_string(fnName)) : NULL; // FIXME
	// filename is the second argument, so ignore.
	//return(Evaluators::reduce(AST::makeApplication(body, argument)));
	// TODO prevent crash if get_symbol_name returned NULL
	return(new CProcedure(nativeProc, AST::makeApplication(AST::makeApplication(AST::makeApplication(AST::symbolFromStr("requireSharedLibrary"), libName), quote(signature)), fnName), strlen(AST::get_symbol_name(signature)) - 2 + 1/*monad*/, 0, signature));
}
AST::NodeT wrapLoadLibraryC(AST::NodeT nameS) {
	const char* name = Evaluators::get_string(nameS);
	void* clib = dlopen(name, RTLD_LAZY);
	if(!clib) {
		fprintf(stderr, "(dlopen \"%s\") failed because: %s\n", name, dlerror());
	}
	return(AST::makeBox(clib, AST::makeApplication(&SharedLibraryLoader, nameS)));
	//return(AST::makeAbstraction(AST::symbolFromStr("name"), result));
}
static AST::NodeT wrapLoadLibrary(AST::NodeT options, AST::NodeT filename) {
	filename = Evaluators::reduce(filename);
	//Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, filename);
	//Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	//Evaluators::CXXArguments::const_iterator endIter = arguments.end();
	AST::NodeT body = wrapLoadLibraryC(filename);
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
bool sharedLibraryFileP(const char* name) {
	FILE* input_file;
	char buf[4];
	input_file = fopen(name, "rb");
	if(!input_file)
		return(false);
	if(fread(buf, 3, 1, input_file) != 1) {
		fclose(input_file);
		return(false);
	}
	fclose(input_file);
	return(strncmp(buf, "ELF\106", 4) == 0);
}

};
