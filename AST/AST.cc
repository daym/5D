/*
4D vector analysis program
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
std::string Literal::str(void) const {
	return(text);
}
std::string StringLiteral::str(void) const {
	return(std::string("\"") + text + "\""); // FIXME escape.
}
std::string Cons::str(void) const {
	std::stringstream result;
	Cons* tail = this->tail;
	result << '(';
	assert(head);
	result << head->str();
	for(; tail; tail = tail->tail) {
		assert(tail->head);
		result << ' ' << tail->head->str();
	}
	result << ')';
	return(result.str());
}
Cons* cons(Node* head, Cons* tail) {
	Cons* result = new Cons;
	assert(head);
	result->head = head;
	result->tail = tail;
	return(result);
}
Literal* literal(const char* text) {
	Literal* result = new Literal;
	result->text = text;
	return(result);
}
StringLiteral* string_literal(const char* text) {
	StringLiteral* result = new StringLiteral;
	result->text = strdup(text);
	return(result);
}

}; /* end namespace AST */

