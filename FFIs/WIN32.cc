/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <windows.h>
#include "Evaluators/FFI"
#include "FFIs/WIN32"

namespace FFIs {

using namespace Evaluators;

struct CP {
	const char* symbol_name;
	const char* library_name;
	const char* signature;
	HMODULE library;
	void* value;
};

C::C(const char* symbol, const char* signature, const char* library) {
	this->p = new CP();
	p->symbol_name = symbol;
	p->library_name = library;
	p->library = LoadLibrary(library, RTLD_LAZY); /* TODO cache */
	p->value = GetProcAddress(p->library, symbol);
}

AST::Node* C::execute(AST::Node* argument) {
	return(NULL);
}

};
