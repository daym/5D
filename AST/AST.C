#include "AST/AST"

namespace AST {

std::string str(const Node& a) {
	return("<node>");
}

std::string str(const Cons& a) {
	return("<cons>");
}

Cons* cons(Node* head, Cons* tail) {
	Cons* result = new Cons;
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

