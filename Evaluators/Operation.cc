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
#include "AST/HashTable"
#include "Evaluators/Operation"
#include "Evaluators/Evaluators"
#include "FFIs/VariantPacker"

namespace Trampolines {

typedef AST::NodeT (jumper_t)(Evaluators::CProcedure* p2, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& iter, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& end, AST::NodeT options, AST::NodeT world);
AST::NodeT jumpFFI(Evaluators::CProcedure* p2, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& iter, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& end, AST::NodeT options, AST::NodeT world); 
// do NOT gc_allocate the following since it seems to have a bug:
typedef AST::RawHashTable<const char*, jumper_t*, AST::hashstr, AST::eqstr> HashTable;
#ifdef WIN32
AST::NodeT jumpFFI(Evaluators::CProcedure* proc, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& iter, std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator& endIter, AST::NodeT options, AST::NodeT world) {
	std::string v = Evaluators::str(proc->fSignature);
	fprintf(stderr, "warning: could not find marshaller for %s\n", v.c_str());
	return Evaluators::makeIOMonad(NULL, endIter->second);
}
#endif

};
#include "FFIs/Trampolines"

namespace Evaluators {

CProcedure::CProcedure(void* native, AST::NodeT aRepr, int aArgumentCount, int aReservedArgumentCount, AST::NodeT aSignature) : 
	AST::Box(native, aRepr),
	fArgumentCount(aArgumentCount),
	fReservedArgumentCount(aReservedArgumentCount),
	fSignature(aSignature)
{
	assert(fReservedArgumentCount == 0 || fReservedArgumentCount == 1);
	AST::Symbol* aSym = dynamic_cast<AST::Symbol*>(aRepr);
	std::string v = aSym ? aSym->name : str(aRepr);
	if(v == "<node>")
		abort();
}
REGISTER_STR(CurriedOperation, {
	std::stringstream sst;
	sst << "(" << str(node->fOperation) << " " << str(node->fArgument) << " " << ")";
	return(sst.str());
})

bool builtin_call_done_P(AST::NodeT node) {
	return(true);
}
bool builtin_call_P(AST::NodeT node) {
	return(dynamic_cast<CProcedure*>(node) || dynamic_cast<CurriedOperation*>(node));
}
AST::NodeT call_builtin(AST::NodeT fn, AST::NodeT argument) {
	AST::NodeT proc1 = fn;
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
		// TODO throw Evaluators::EvaluationException("could not reduce");
		return(AST::makeApplication(replace(proc2, proc2->fRepr, fn), argument)); // TODO exception
	}
	if(argumentCount != proc2->fArgumentCount && argumentCount != -proc2->fArgumentCount) {
		return Evaluators::makeCurriedOperation(fn, argument);
	}
	//printf("call %p\n", proc2->native);
	if(nil_P(proc2->fSignature)) { // probably wants the arguments unevaluated, so stop messing with them.
		AST::NodeT (*proc3)(AST::NodeT, AST::NodeT) = (AST::NodeT (*)(AST::NodeT, AST::NodeT)) proc2->native;
		return((*proc3)(fn, argument));
	} else
		return(Trampolines::jumpT(proc2, fn, argument));
}

AST::NodeT repr(AST::NodeT node) {
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
		AST::NodeT fn = get_application_operator(node);
		AST::NodeT argument = get_application_operand(node);
		//if(fn == &Evaluators::Reducer && application_P(argument)) { // special case to get rid of implicit repl FIXME FIXME dangerous
		//	return(repr(get_application_operator(argument)));
		//}
		AST::NodeT new_fn = repr(fn);
		AST::NodeT new_argument = repr(argument);
		if(new_fn == fn && new_argument == argument)
			return(node);
		else
			return(AST::makeApplication(fn, argument));
		// TODO repr hashtable for dispatchmodule as a simple list.
	} else
		return(node);
}


}; /* end namespace Evaluators */
