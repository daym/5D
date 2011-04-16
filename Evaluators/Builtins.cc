#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"

namespace Evaluators {

struct Quoter : Operation {
	virtual bool eager_P(void) const;
	virtual AST::Node* execute(AST::Node* argument);
};
struct ProcedureP : Operation {
	virtual AST::Node* execute(AST::Node* argument);
};

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
