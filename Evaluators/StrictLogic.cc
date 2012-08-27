#define USE_SPECIAL_FORMS
#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/Logic"
#include "Evaluators/Builtins"

namespace Evaluators {

static inline bool false_P(AST::NodeT node) {
	return(node == aFalse);
}

static AST::NodeT sequence2(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	Evaluators::reduce(a);
	return(Evaluators::reduce(b));
}
static AST::NodeT and2(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	AST::NodeT result = Evaluators::reduce(a);
	if(false_P(result))
		return(result);
	return(Evaluators::reduce(b));
}
static AST::NodeT or2(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	AST::NodeT result = Evaluators::reduce(a);
	if(!false_P(result))
		return(result);
	return(Evaluators::reduce(b));
}

DEFINE_BINARY_SPECIAL_FORM(Sequencer, sequence2)
DEFINE_BINARY_SPECIAL_FORM(Ander, and2)
DEFINE_BINARY_SPECIAL_FORM(Orer, or2)
DEFINE_SPECIAL_FORM(Noter, false_P(argument))
static AST::NodeT wrapIf(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	AST::NodeT condition = iter->second;
	++iter;
	AST::NodeT trueBranch = iter->second;
	++iter;
	AST::NodeT falseBranch = iter->second;
	++iter;
	bool B_conditionFalse = false_P(Evaluators::reduce(condition));
	return(Evaluators::reduce(B_conditionFalse ? falseBranch : trueBranch));
}
DEFINE_FULL_OPERATION(Ifer, {
	return(wrapIf(fn, argument));
})

REGISTER_BUILTIN(Sequencer, 2, 0, AST::symbolFromStr(";"))
REGISTER_BUILTIN(Ander, 2, 0, AST::symbolFromStr("&&"))
REGISTER_BUILTIN(Orer, 2, 0, AST::symbolFromStr("||"))
REGISTER_BUILTIN(Noter, 1, 0, AST::symbolFromStr("not"))
REGISTER_BUILTIN(Ifer, 3, 0, AST::symbolFromStr("if"))

}; /* end namespace Evaluators */

