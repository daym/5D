/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <windows.h>
#include "stdafx.h"
#include "Evaluators/FFI"
#include "FFIs/WIN32"

namespace FFIs {

using namespace Evaluators;

struct CP {
	AST::Symbol* fn_name;
	AST::Symbol* library_name;
	AST::Symbol* signature;
	HMODULE library;
	FARPROC value;
};

C::C(AST::Symbol* fn, AST::Symbol* signature, AST::Symbol* library) {
	this->p = new CP();
	p->fn_name = fn;
	p->library_name = library;
	std::wstring library_W = FromUTF8(library->name);
	p->library = LoadLibrary(library_W.c_str()); /* TODO cache */
	p->value = GetProcAddress(p->library, fn->name);
}

AST::Node* C::executeLowlevel(AST::Node* argument) {
	return(NULL);
}

};
