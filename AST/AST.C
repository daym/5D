#include <assert.h>
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

}; /* end namespace AST */

