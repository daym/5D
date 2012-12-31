#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <assert.h>
#include <string>
#include <string.h>
#include <sstream>
#include <5D/Operations>
#include "Values/Values"
#include "Values/Symbols"
#include "Evaluators/Evaluators"
#include "FFIs/VariantPacker"
#include "Evaluators/Builtins"

namespace Trampolines {
using namespace Values;

typedef NodeT (jumper_t)(Evaluators::CProcedure* p2, Evaluators::CXXArguments::const_iterator& iter, Evaluators::CXXArguments::const_iterator& end, NodeT options, NodeT world);
NodeT jumpFFI(Evaluators::CProcedure* p2, Evaluators::CXXArguments::const_iterator& iter, Evaluators::CXXArguments::const_iterator& end, NodeT options, NodeT world); 
// do NOT gc_allocate the following since it seems to have a bug:
typedef RawHashTable<const char*, jumper_t*, hashstr, eqstr> HashTable;
#ifdef WIN32
NodeT jumpFFI(Evaluators::CProcedure* proc, Evaluators::CXXArguments::const_iterator& iter, Evaluators::CXXArguments::const_iterator& endIter, NodeT options, NodeT world) {
	std::string v = Evaluators::str(proc->fSignature);
	fprintf(stderr, "warning: could not find marshaller for %s\n", v.c_str());
	//FXXXETCH_WORLD1(endIter);
	return CHANGED_WORLD(NULL);
}
#endif

};
#include "FFIs/Trampolines"

namespace Evaluators {
using namespace Values;

CProcedure::CProcedure(void* value, NodeT aRepr, int aArgumentCount, int aReservedArgumentCount, NodeT aSignature) : 
	Box(value, aRepr),
	fArgumentCount(aArgumentCount),
	fReservedArgumentCount(aReservedArgumentCount),
	fSignature(aSignature)
{
	assert(fReservedArgumentCount == 0 || fReservedArgumentCount == 1);
	//Symbol* aSym = dynamic_cast<Symbol*>(aRepr);
	//std::string v = aSym ? aSym->name : str(aRepr);
	//if(v == "<node>")
	//	abort();
}
REGISTER_STR(CurriedOperation, {
	std::stringstream sst;
	sst << "(" << str(node->fOperation) << " " << str(node->fArgument) << " " << ")";
	return(sst.str());
})

bool builtin_call_done_P(NodeT node) {
	return(true);
}
bool builtin_call_P(NodeT node) {
	return(dynamic_cast<CProcedure*>(node) || curried_operation_P(node));
}
NodeT call_builtin(NodeT fn, NodeT argument) {
	NodeT proc1 = fn;
	CProcedure* proc2;
	NodeT c;
	bool inKV = false;
	int argumentCount = !keyword_P(argument) ? 1 : (-1); // number of non-keyword arguments.
	bool B_had_keyword_arguments = false;
	while(Evaluators::curried_operation_P(proc1)) {
		c = proc1;
		if(!inKV && keyword_P(Evaluators::get_curried_operation_argument(c))) {
			inKV = true;
			--argumentCount;
			B_had_keyword_arguments = true;
		} else {
			inKV = false;
			++argumentCount;
		}
		proc1 = Evaluators::get_curried_operation_operation(c);
	}
	if(argumentCount < 0)
		argumentCount = 0;
	proc2 = dynamic_cast<CProcedure*>(proc1);
	assert(proc2);
	if(B_had_keyword_arguments && proc2->fArgumentCount >= 0) { /* not allowed */
		// TODO throw Evaluators::EvaluationException("could not reduce");
		return(makeApplication(replace(proc2, proc2->fRepr, fn), argument)); // TODO exception
	}
	if(argumentCount != proc2->fArgumentCount && argumentCount != -proc2->fArgumentCount) {
		return Evaluators::makeCurriedOperation(fn, argument);
	}
	//printf("call %p\n", proc2->native);
	if(nil_P(proc2->fSignature)) { // probably wants the arguments unevaluated, so stop messing with them.
		NodeT (*proc3)(NodeT, NodeT) = (NodeT (*)(NodeT, NodeT)) proc2->value;
		return((*proc3)(fn, argument));
	} else
		return(Trampolines::jumpT(proc2, fn, argument));
}

NodeT repr(NodeT node) {
	Evaluators::CProcedure* operation;
	if((operation = dynamic_cast<Evaluators::CProcedure*>(node)) != NULL) {
		return(operation->fRepr);
	} else if(Evaluators::curried_operation_P(node)) {
		// this is a special case and really should be generalized. FIXME.
		NodeT operation_ = Evaluators::get_curried_operation_operation(node);
		CProcedure* p = dynamic_cast<CProcedure*>(operation_);
		if(p && p->fReservedArgumentCount > 0)
			return(repr(operation_));
		else
			return(makeApplication(repr(operation_), repr(Evaluators::get_curried_operation_argument(node))));
	} else if(application_P(node)) {
		NodeT fn = get_application_operator(node);
		NodeT argument = get_application_operand(node);
		//if(fn == &Evaluators::Reducer && application_P(argument)) { // special case to get rid of implicit repl FIXME FIXME dangerous
		//	return(repr(get_application_operator(argument)));
		//}
		NodeT new_fn = repr(fn);
		NodeT new_argument = repr(argument);
		if(new_fn == fn && new_argument == argument)
			return(node);
		else
			return(makeApplication(fn, argument));
		// TODO repr hashtable for dispatchmodule as a simple list.
	} else if(abstraction_P(node)) {
		NodeT parameter = get_abstraction_parameter(node);
		NodeT body = get_abstraction_body(node);
		NodeT new_parameter = /*repr*/(parameter);
		NodeT new_body = repr(body);
		if(new_parameter == parameter && new_body == body)
			return(node);
		else
			return(makeAbstraction(parameter, body));
	} else
		return(node);
	/* FIXME abstraction */
}


}; /* end namespace Evaluators */
