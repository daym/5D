#include <stdio.h>
#include <assert.h>
#include <string>
#include <sstream>
#include "AST/AST"
#include "AST/Keyword"
#include "Evaluators/Operation"

namespace Evaluators {

CProcedure::CProcedure(void* native, AST::Node* aRepr, int aArgumentCount) : 
	AST::Box(native),
	fRepr(aRepr),
	fArgumentCount(aArgumentCount)
{
}
REGISTER_STR(CProcedure, return(str(node->fRepr));)
REGISTER_STR(CurriedOperation, {
	std::stringstream sst;
	sst << "(" << str(node->fOperation) << " " << str(node->fArgument) << " " << ")";
	return(sst.str());
})

bool builtin_call_done_P(AST::Node* node) {
	return(true);
}
bool builtin_call_P(AST::Node* node) {
	return(dynamic_cast<CProcedure*>(node) || dynamic_cast<CurriedOperation*>(node));
}
AST::Node* call_builtin(AST::Node* fn, AST::Node* argument) {
	AST::Node* proc1 = fn;
	CProcedure* proc2;
	CurriedOperation* c;
	int argumentCount = 1;
	while((c = dynamic_cast<CurriedOperation*>(proc1)) != NULL) {
		//printf("%s\n", str(c->fArgument).c_str());
		if(keyword_P(c->fArgument)) {
			--argumentCount;
		} else
			++argumentCount;
		proc1 = c->fOperation;
	}
	proc2 = dynamic_cast<CProcedure*>(proc1);
	assert(proc2);
	if(argumentCount != proc2->fArgumentCount && argumentCount != -proc2->fArgumentCount) {
		return new CurriedOperation(fn, argument);
	}
	AST::Node* (*proc3)(AST::Node*, AST::Node*) = (AST::Node* (*)(AST::Node*, AST::Node*)) proc2->native;
	return((*proc3)(fn, argument));
}

AST::Node* repr(AST::Node* node) {
	Evaluators::CProcedure* operation;
	Evaluators::CurriedOperation* c;
	if((operation = dynamic_cast<Evaluators::CProcedure*>(node)) != NULL) {
		return(operation->fRepr);
	} else if((c = dynamic_cast<Evaluators::CurriedOperation*>(node)) != NULL) {
		return(AST::makeApplication(repr(c->fOperation), repr(c->fArgument)));
	} else
		return(node);
}

}; /* end namespace Evaluators */
