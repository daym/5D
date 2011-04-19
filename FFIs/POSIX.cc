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

namespace FFIs {
using namespace Evaluators;
static std::map<AST::Symbol*, void*> known_libraries;
struct CP {
	AST::Symbol* fn_name;
	AST::Symbol* library_name;
	AST::Symbol* signature;
	void* library;
	void* value;
};
C::C(AST::Symbol* fn, AST::Symbol* signature, AST::Symbol* library, bool B_pure) {
	this->p = new CP();
	p->fn_name = fn;
	p->library_name = library;
	p->signature = signature;
	this->B_pure = B_pure;
	std::map<AST::Symbol*, void*>::const_iterator library_iter = known_libraries.find(library);
	if(library_iter == known_libraries.end()) {
		p->library = dlopen(library->name, RTLD_LAZY);
		known_libraries[library] = p->library;
	} else
		p->library = library_iter->second;
	dlerror(); /* clear errors */
	p->value = dlsym(p->library, fn->name);
	assert(p->value);
}
static AST::Node* call2(AST::Symbol* signature, void* fn, AST::Node* a, AST::Node* b) {
	/* TODO: important ones: 
	     pp
	     pii
	     ip
	     pi
	     ppp
	     ii
	     ?
	     */
	/* FIXME */
	if(signature == AST::intern("pp")) {
		return(intern(((int (*) (void*, void*)) fn)( get_native_pointer(a), get_native_pointer(b) )));
	} else if(signature == AST::intern("pi")) {
		return(intern(((int (*) (void*, int)) fn)( get_native_pointer(a), get_native_integer(b) )));
	} else if(signature == AST::intern("ip")) {
		return(intern(((int (*) (int, void*)) fn)( get_native_integer(a), get_native_pointer(b) )));
	} else if(signature == AST::intern("ii")) {
		return(intern(((int (*) (int, int)) fn)( get_native_integer(a), get_native_integer(a) )));
	} else {
		std::stringstream sst;
		sst << "warning: unknown signature \"" << signature->name << "\". Not calling FF.";
		throw EvaluationException(strdup(sst.str().c_str())); // FIXME fix memory leak.
		return(NULL);
	}
}
AST::Node* C::executeLowlevel(AST::Node* argument) {
	if(p->signature == AST::intern("v"))
		return(intern(((int (*) (void)) p->value)()));
	else if(p->signature == AST::intern("i"))
		return(intern(((int (*) (int)) p->value)( get_native_integer(argument) )));
	else if(p->signature == AST::intern("p"))
		return(intern(((int (*) (void*)) p->value)( get_native_pointer(argument) )));
	else if(p->signature == AST::intern("?"))
		return(intern(((int (*) (bool)) p->value)( get_native_boolean(argument) )));
	else {
		std::stringstream sst;
		sst << "warning: unknown signature \"" << p->signature->name << "\". Not calling FF.";
		throw EvaluationException(strdup(sst.str().c_str())); // FIXME fix memory leak.
		return(NULL);
	}
}

};
