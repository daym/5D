#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "FFIs/POSIX"

namespace Evaluators {

bool Quoter::eager_P(void) const {
	return(false);
}
AST::Node* Quoter::execute(AST::Node* argument) {
	AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(argument);
	return(ref ? ref->symbol : argument); /* prevent evaluation by destroying the symbol reference */
}
AST::Node* ProcedureP::execute(AST::Node* argument) {
	if(argument != NULL && (dynamic_cast<Operation*>(argument) != NULL))
		return(argument); /* FIXME return true or false */
	else
		return(NULL);
}
AST::Node* internNative(int value) {
	/* FIXME */
	return(NULL);
}
AST::Node* internNative(bool value) {
	/* FIXME */
	return(NULL);
}
Conser2::Conser2(AST::Node* head) {
	this->head = head;
}
AST::Node* Conser2::execute(AST::Node* argument) {
	/* FIXME error message if it doesn't work. */
	return(cons(head, dynamic_cast<AST::Cons*>(argument)));
}
AST::Node* Conser::execute(AST::Node* argument) {
	return(new Conser2(argument));
}
AST::Node* ConsP::execute(AST::Node* argument) {
	bool result = dynamic_cast<AST::Cons*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* HeadGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->head);
	else
		return(NULL); // FIXME proper error message!
}
AST::Node* TailGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->tail);
	else
		return(NULL); // FIXME proper error message!
}

}; /* end namespace Evaluators */
