/*
4D vector analysis program
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
#include "FFIs/POSIX"
#include "FFIs/Trampoline"

namespace FFIs {
using namespace Evaluators;

struct LibraryLoaderP {
	std::map<AST::Symbol*, CLibrary*> knownLibraries;
};
LibraryLoader::LibraryLoader(void) {
	p = new LibraryLoaderP();
}
AST::Node* LibraryLoader::executeLowlevel(AST::Node* libraryName) {
	AST::Symbol* libraryNameSymbol = dynamic_cast<AST::Symbol*>(libraryName);
	if(libraryNameSymbol == NULL)
		return(NULL);
	std::map<AST::Symbol*, CLibrary*>::const_iterator iter = p->knownLibraries.find(libraryNameSymbol);
	if(iter != p->knownLibraries.end())
		return(iter->second);
	else {
		p->knownLibraries[libraryNameSymbol] = new CLibrary(libraryNameSymbol->name);
		return(p->knownLibraries[libraryNameSymbol]);
	}
}
struct CLibraryP {
	void* library;
	std::map<AST::Symbol*, CProcedure*> knownProcedures;
};
CLibrary::CLibrary(const char* name) {
	p = new CLibraryP();
	p->library = dlopen(name, RTLD_LAZY);
}
AST::Node* CLibrary::executeLowlevel(AST::Node* argument) {
	/* argument is the name (symbol). Result is a CProcedure */
	AST::Symbol* nameSymbol = dynamic_cast<AST::Symbol*>(argument);
	if(nameSymbol == NULL)
		return(NULL);
	std::map<AST::Symbol*, CProcedure*>::const_iterator iter = p->knownProcedures.find(nameSymbol);
	if(iter != p->knownProcedures.end())
		return(iter->second);
	else {
		p->knownProcedures[nameSymbol] = new CProcedure(dlsym(p->library, nameSymbol->name));
		return(p->knownProcedures[nameSymbol]);
	}
}
CProcedure::CProcedure(void* native) : 
	AST::Box(native)
{
}

};
