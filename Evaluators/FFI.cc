/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Numbers/Integer"

namespace Evaluators {
using namespace AST;
using namespace Numbers;

int get_native_integer(AST::Node* root) {
	Int* rootInt = dynamic_cast<Int*>(root);
	if(rootInt) {
		return(rootInt->value);
	}
	/* FIXME */
	return(0);
}
void* get_native_pointer(AST::Node* root) {
	/* FIXME */
	return(NULL);
}
bool get_native_boolean(AST::Node* root) {
	/* FIXME */
	return(false);
}
char* get_native_string(AST::Node* root) {
	AST::Str* rootString = dynamic_cast<AST::Str*>(root);
	if(rootString)
		return(strdup(rootString->text.c_str()));
	else {
		std::string value = str(root); /* FIXME */
		return(strdup(value.c_str()));
	}
}

}; /* end namspace */
