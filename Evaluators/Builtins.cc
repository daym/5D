#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"

namespace Evaluators {

struct Quoter : Operation {
	virtual bool eager_P(void) const;
	virtual AST::Node* execute(AST::Node* argument);
};
bool Quoter::eager_P(void) const {
	return(false);
}
AST::Node* Quoter::execute(AST::Node* argument) {
	return(argument);
}

AST::Node* intern(int value) {
	/* FIXME */
	return(NULL);
}

}; /* end namespace Evaluators */
