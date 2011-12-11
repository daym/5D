#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <assert.h>
#include <string>
#include <string.h>
#include <sstream>
#include "AST/AST"
#include "AST/Keyword"
#include "AST/Symbols"
#include "Evaluators/Operation"
#include "Evaluators/Evaluators"
#include <ext/hash_map>
namespace Trampolines {
//typedef std::unordered_map<const char* , AST:Node*, std::hash<AST::Node*> > HashTable;
struct eqstr {
	bool operator()(const char* s1, const char* s2) const {
		return strcmp(s1, s2) == 0;
	}
};

typedef AST::Node* (jumper_t)(void* p, std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator& iter, std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator& end);
typedef __gnu_cxx::hash_map<const char*, jumper_t*, __gnu_cxx::hash<const char*>, eqstr> HashTable;
};
#include "FFIs/Trampolines"

namespace Evaluators {

CProcedure::CProcedure(void* native, AST::Node* aRepr, int aArgumentCount, int aReservedArgumentCount, AST::Symbol* aSignature) : 
	AST::Box(native),
	fRepr(aRepr),
	fArgumentCount(aArgumentCount),
	fReservedArgumentCount(aReservedArgumentCount),
	fSignature(aSignature)
{
	assert(fReservedArgumentCount == 0 || fReservedArgumentCount == 1);
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
	bool inKV = false;
	int argumentCount = !keyword_P(argument) ? 1 : (-1); // number of non-keyword arguments.
	bool B_had_keyword_arguments = false;
	while((c = dynamic_cast<CurriedOperation*>(proc1)) != NULL) {
		if(!inKV && keyword_P(c->fArgument)) {
			inKV = true;
			--argumentCount;
			B_had_keyword_arguments = true;
		} else {
			inKV = false;
			++argumentCount;
		}
		proc1 = c->fOperation;
	}
	if(argumentCount < 0)
		argumentCount = 0;
	proc2 = dynamic_cast<CProcedure*>(proc1);
	assert(proc2);
	if(B_had_keyword_arguments && proc2->fArgumentCount >= 0) { /* not allowed */
		return(AST::makeApplication(replace(proc2, proc2->fRepr, fn), argument));
	}
	if(argumentCount != proc2->fArgumentCount && argumentCount != -proc2->fArgumentCount) {
		return Evaluators::makeCurriedOperation(fn, argument);
	}
	if(proc2->fSignature == NULL) { // probably wants the arguments unevaluated, so stop messing with them.
		AST::Node* (*proc3)(AST::Node*, AST::Node*) = (AST::Node* (*)(AST::Node*, AST::Node*)) proc2->native;
		return((*proc3)(fn, argument));
	} else
		return(Trampolines::jumpT(proc2, fn, argument));
}

AST::Node* repr(AST::Node* node) {
	Evaluators::CProcedure* operation;
	Evaluators::CurriedOperation* c;
	if((operation = dynamic_cast<Evaluators::CProcedure*>(node)) != NULL) {
		return(operation->fRepr);
	} else if((c = dynamic_cast<Evaluators::CurriedOperation*>(node)) != NULL) {
		// this is a special case and really should be generalized. FIXME.
		CProcedure* p = dynamic_cast<CProcedure*>(c->fOperation);
		if(p && p->fReservedArgumentCount > 0)
			return(repr(c->fOperation));
		else
			return(AST::makeApplication(repr(c->fOperation), repr(c->fArgument)));
	} else if(application_P(node)) {
		AST::Node* fn = get_application_operator(node);
		AST::Node* argument = get_application_operand(node);
		//if(fn == &Evaluators::Reducer && application_P(argument)) { // special case to get rid of implicit repl FIXME FIXME dangerous
		//	return(repr(get_application_operator(argument)));
		//}
		AST::Node* new_fn = repr(fn);
		AST::Node* new_argument = repr(argument);
		if(new_fn == fn && new_argument == argument)
			return(node);
		else
			return(AST::makeApplication(fn, argument));
	} else
		return(node);
}


}; /* end namespace Evaluators */
