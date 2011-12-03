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
#include "FFIs/FFIs"
#include "AST/Symbols"
/*include "FFIs/Trampoline"*/

namespace FFIs {
using namespace Evaluators;

struct LibraryLoaderP {
	std::map<AST::Symbol*, CLibrary*> knownLibraries;
};
LibraryLoader::LibraryLoader(AST::Node* fallback) {
	p = new LibraryLoaderP();
}
CLibrary* LibraryLoader::execute(AST::Node* libraryName) {
	if(str_P(libraryName))
		libraryName = AST::symbolFromStr(((AST::Str*)libraryName)->text.c_str());
	AST::Symbol* libraryNameSymbol = dynamic_cast<AST::Symbol*>(libraryName);
	if(libraryNameSymbol == NULL)
		return(NULL);
	std::map<AST::Symbol*, CLibrary*>::const_iterator iter = p->knownLibraries.find(libraryNameSymbol);
	if(iter != p->knownLibraries.end())
		return(iter->second);
	else {
		CLibrary* library;
		library = new CLibrary(libraryNameSymbol->name);
		if(library->goodP()) {
			p->knownLibraries[libraryNameSymbol] = library;
			return(p->knownLibraries[libraryNameSymbol]);
		} else {
			return(library); // junk
		}
	}
}
struct CLibraryP {
	void* library;
	std::string name;
	std::map<AST::Symbol*, CProcedure*> knownProcedures;
};
bool CLibrary::goodP() const {
	return(p->library != NULL);
}
CLibrary::CLibrary(const char* name) {
	p = new CLibraryP();
	p->name = name;
	p->library = dlopen(name, RTLD_LAZY);
	if(!p->library) {
		fprintf(stderr, "(dlopen \"%s\") failed because: %s\n", name, dlerror());
		//perror(name);
	}
}
AST::Node* CLibrary::executeLowlevel(AST::Node* argument, int argumentCount) {
	/* argument is the name (symbol). Result is a CProcedure */
	if(str_P(argument))
		argument = AST::symbolFromStr(((AST::Str*)argument)->text.c_str());
	AST::Symbol* nameSymbol = dynamic_cast<AST::Symbol*>(argument);
	if(nameSymbol == NULL)
		return(NULL);
	std::map<AST::Symbol*, CProcedure*>::const_iterator iter = p->knownProcedures.find(nameSymbol);
	if(iter != p->knownProcedures.end())
		return(iter->second);
	else {
		void* proc = dlsym(p->library, nameSymbol->name);
		AST::Node* fRepr = AST::makeApplication(AST::makeApplication(Symbols::SfromLibrary, AST::makeStr(p->name.c_str())), AST::makeApplication(Symbols::Squote, nameSymbol));
		if(!proc) {
			fprintf(stderr, "error: could not find symbol \"%s\" in library \"%s\"\n", nameSymbol->name, p->name.c_str());
			return(fRepr);
		}
		p->knownProcedures[nameSymbol] = new CProcedure(proc, fRepr, argumentCount);
		return(p->knownProcedures[nameSymbol]);
	}
}
REGISTER_STR(CLibrary, return(str(makeApplication(Symbols::SfromLibrary, AST::makeStr(node->p->name.c_str()))));)

};
