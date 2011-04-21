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

typedef enum {
	BT_NONE,
	BT_OBJECT,
	BT_INT,
	BT_STRING,
} NativeBaseType;

struct CP {
	AST::Symbol* fn_name;
	AST::Symbol* library_name;
	AST::Symbol* signature;
	HMODULE library;
	FARPROC value;
	NativeBaseType argument_types[16];
	/* TODO int minimum_argument_count; */
};

C::C(AST::Symbol* fn, AST::Symbol* signature, AST::Symbol* library, bool B_pure) {
	this->p = new CP();
	this->B_pure = B_pure;
	p->fn_name = fn;
	p->library_name = library;
	std::wstring library_W = FromUTF8(library->name);
	p->library = LoadLibrary(library_W.c_str()); /* TODO cache */
	p->value = GetProcAddress(p->library, fn->name);
	p->signature = signature;
	for(int i = 0; i < 16; ++i)
		p->argument_types[i] = BT_NONE;
}

/* TODO Make this FPU-safe, X8664-safe. Long term, use the Compiler (see "Compilers").  */
static void* use_main_trampoline(void* fn, void* a1, void* a2, void* a3, void* a4, void* a5, void* a6, void* a7, void* a8, void* a9, void* a10, void* a11, void *a12, void* a13, void* a14, void* a15, void* a16) {
	__asm {
		pop eax
		call eax
		push eax
	}
}
/* EEW. */
static void* get_native(NativeBaseType t, AST::Node* argument) {
	return(t == BT_NONE ? NULL : 
		   t == BT_OBJECT ? (void*) get_native_pointer(argument) : 
		   t == BT_INT ? (void*) get_native_integer(argument) :
		   t == BT_STRING ? (void*) get_native_string(argument) :
		   NULL);
}
AST::Node* C::executeLowlevel(AST::Node* argument) {
	void* result;
	void* a[16] = {0};
	AST::Cons* args = dynamic_cast<AST::Cons*>(argument);
	if(args) {
		int i;
		for(i = 0; i < 16; ++i, args = args->tail) {
			a[i] = get_native(p->argument_types[i], args->head);
		}
	} else 
		a[0] = get_native(p->argument_types[0], argument);
	/* TODO minimum_argument_count */
	result = use_main_trampoline(p->value, a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7], a[8], a[9], a[10], a[11], a[12], a[13], a[14], a[15]);
	return(NULL); /* FIXME */
}

};
