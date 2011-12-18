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
#include "AST/Symbols"

namespace AST {

Node::~Node() {
}

Cons* makeCons(Node* head, Node* tail) {
	Cons* result = new Cons;
	result->head = head;
	result->tail = tail;
	return(result);
}
Str* makeStrRaw(char* mutableText, size_t size) {
	if(size < 1)
		return(NULL);
	AST::Str* result = new AST::Str((void*) mutableText);
	result->size = size;
	return(result);
}
Str* makeStr(const char* text) {
	if(strlen(text) < 1)
		return(NULL);
	else
		return(makeStrRaw(GC_strdup(text), strlen(text)));
}
Str* makeStrCXX(const std::string& text) {
	if(text.length() == 0)
		return(NULL);
	char* result = new (UseGC) char[text.length() + 1];
	memcpy(result, text.c_str(), text.length() + 1);
	return(makeStrRaw(result, text.length()));
}
bool str_P(AST::Node* node) {
	return(dynamic_cast<AST::Str*>(node) != NULL);
}

Application* makeApplication(Node* fn, Node* argument) {
	Application* result = new Application;
	result->operator_ = fn;
	result->operand = argument;
	result->result = NULL;
	result->resultGeneration = 0;
	return(result);
}
Abstraction* makeAbstraction(Node* parameter, Node* body) {
	Abstraction* result = new Abstraction;
	result->parameter = parameter;
	result->body = body;
	return(result);
}
Application* makeOperation(Node* operator_, Node* operand_1, Node* operand_2) {
	if(operator_ == NULL/* || operand_1 == NULL*//* || operand_2 == NULL*/) {
		return(NULL);
	} else if(operator_ == Symbols::Sspace) // apply
		return(makeApplication(operand_1, operand_2));
	else
		return(makeApplication(makeApplication(operator_, operand_1), operand_2));
}

}; /* end namespace AST */

