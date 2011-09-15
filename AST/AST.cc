/*
5D vector analysis program
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

namespace AST {

std::string Node::str(void) const {
	return("<node>");
}
std::string Atom::str(void) const {
	return(text);
}
std::string String::str(void) const {
	std::stringstream sst;
	const char* item;
	char c;
	sst << "\"";
	for(item = text.c_str(); (c = *item); ++item) {
		if(c == '"')
			sst << '\\';
		else if(c == '\\')
			sst << '\\';
		/* TODO escape other things? not that useful... */
		sst << c;
	}
	sst << "\"";
	return(sst.str());
}
std::string Box::str(void) const {
	return("box"); // TODO nicer?
}
std::string Cons::str(void) const {
	std::stringstream result;
	Cons* tail = this->tail;
	result << '(';
	result << (head ? head->str() : "()");
	for(; tail; tail = tail->tail) {
		result << ' ' << (tail->head ? tail->head->str() : "()");
	}
	result << ')';
	return(result.str());
}
Cons* cons(Node* head, Cons* tail) {
	Cons* result = new Cons;
	/*assert(head); unfortunately, now that we have NIL, that's allowed. */
	result->head = head;
	result->tail = tail;
	return(result);
}
Atom* literal(const char* text) {
	Atom* result = new Atom;
	result->text = text;
	return(result);
}
String* string_literal(const char* text) {
	String* result = new String(text);
	return(result);
}
bool string_P(AST::Node* node) {
	return(dynamic_cast<AST::String*>(node) != NULL);
}
AST::Cons* follow_tail(AST::Cons* list) {
	if(!list)
		return(NULL);
	while(list->tail)
		list = list->tail;
	return(list);
}

// Operation
AST::Node* Operation::repr(AST::Node* selfName) const {
	return(NULL);
}
bool Operation::eager_P(void) const {
	return(false);
}

}; /* end namespace AST */

