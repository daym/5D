/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <assert.h>
#include <string.h>
#include <string>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"

namespace AST {

Node::~Node() {
}

Cons* makeCons(Node* head, Node* tail) {
	Cons* result = new Cons;
	/*assert(head); unfortunately, now that we have NIL, that's allowed. */
	result->head = head;
	result->tail = tail;
	return(result);
}
Str* makeStr(const char* text) {
	Str* result = new Str(text);
	return(result);
}
bool str_P(AST::Node* node) {
	return(dynamic_cast<AST::Str*>(node) != NULL);
}

// Operation
AST::Node* Operation::repr(AST::Node* selfName) const {
	return(NULL);
}
bool Operation::eager_P(void) const {
	return(false);
}
Application* makeApplication(Node* fn, Node* argument) {
	Application* result = new Application;
	result->operator_ = fn;
	result->operand = argument;
	result->result = NULL;
	result->bResult = false;
	return(result);
}
Abstraction* makeAbstraction(Node* parameter, Node* body) {
	Abstraction* result = new Abstraction;
	result->parameter = parameter;
	result->body = body;
	return(result);
}
Application* makeOperation(Node* operator_, Node* operand_1, Node* operand_2) {
	if(operator_ == NULL || operand_1 == NULL/* || operand_2 == NULL*/) {
		return(NULL);
	} else if(operator_ == intern(" ")) // apply
		return(makeApplication(operand_1, operand_2));
	else
		return(makeApplication(makeApplication(operator_, operand_1), operand_2));
}

}; /* end namespace AST */

