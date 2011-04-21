#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "FFIs/POSIX"

namespace Evaluators {


bool Quoter::eager_P(void) const {
	return(false);
}
AST::Node* Quoter::execute(AST::Node* argument) {
	return(argument);
}
AST::Node* ProcedureP::execute(AST::Node* argument) {
	if(argument != NULL && (dynamic_cast<Operation*>(argument) != NULL))
		return(argument); /* FIXME return true or false */
	else
		return(NULL);
}
AST::Node* intern(int value) {
	/* FIXME */
	return(NULL);
}
AST::Node* intern(bool value) {
	/* FIXME */
	return(NULL);
}

}; /* end namespace Evaluators */
