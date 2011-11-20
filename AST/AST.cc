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
#include "AST/Symbol"

namespace Evaluators {
AST::Cons* evaluateToCons(AST::Node* computation);
};

namespace AST {

std::string Node::str(void) const {
	return("<node>");
}
std::string Atom::str(void) const {
	return(text);
}
std::string Str::str(void) const {
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
	result << '[';
	result << (head ? head->str() : "()");
	for(Cons* node = Evaluators::evaluateToCons(this->tail); node; node = Evaluators::evaluateToCons(node->tail)) {
		result << ' ' << (node->head ? node->head->str() : "()");
	}
	result << ']';
	return(result.str());
}
Cons* makeCons(Node* head, Node* tail) {
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
std::string Application::str(void) const {
	std::stringstream result;
	result << '(';
	result << (operator_ ? operator_->str() : std::string("nil"));
	result << ' ';
	result << (operand ? operand->str() : std::string("nil"));
	result << ')';
	return(result.str());
}
std::string Abstraction::str(void) const {
	std::stringstream result;
	result << "(\\";
	result << (parameter ? parameter->str() : std::string("nil"));
	result << (body ? body->str() : std::string("nil"));
	result << ')';
	return(result.str());
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

