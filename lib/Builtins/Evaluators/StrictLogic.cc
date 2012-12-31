#define USE_SPECIAL_FORMS
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "Evaluators/Logic"
#include "Evaluators/Builtins"

namespace Evaluators {

using namespace Values;

NodeT aTrue = symbolFromStr("t");
NodeT aFalse = NULL; // XXX symbolFromStr("f");

NodeT internNative(bool value) {
	return(value ? aTrue : aFalse);
}

static inline bool false_P(NodeT node) {
	return(node == aFalse);
}

static NodeT sequence2(NodeT a, NodeT b, NodeT fallback) {
	Evaluators::reduce(a);
	return(Evaluators::reduce(b));
}
static NodeT and2(NodeT a, NodeT b, NodeT fallback) {
	NodeT result = Evaluators::reduce(a);
	if(false_P(result))
		return(result);
	return(Evaluators::reduce(b));
}
static NodeT or2(NodeT a, NodeT b, NodeT fallback) {
	NodeT result = Evaluators::reduce(a);
	if(!false_P(result))
		return(result);
	return(Evaluators::reduce(b));
}

DEFINE_BINARY_SPECIAL_FORM(Sequencer, sequence2)
DEFINE_BINARY_SPECIAL_FORM(Ander, and2)
DEFINE_BINARY_SPECIAL_FORM(Orer, or2)
DEFINE_SPECIAL_FORM(Noter, false_P(argument))
static NodeT wrapIf(NodeT options, NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	NodeT condition = iter->second;
	++iter;
	NodeT trueBranch = iter->second;
	++iter;
	NodeT falseBranch = iter->second;
	++iter;
	bool B_conditionFalse = false_P(Evaluators::reduce(condition));
	return(Evaluators::reduce(B_conditionFalse ? falseBranch : trueBranch));
}
DEFINE_FULL_OPERATION(Ifer, {
	return(wrapIf(fn, argument));
})
DEFINE_SPECIAL_FORM(Elser, argument)
DEFINE_SPECIAL_FORM(Elifer, argument)

REGISTER_BUILTIN(Sequencer, 2, 0, symbolFromStr(";"))
REGISTER_BUILTIN(Ander, 2, 0, symbolFromStr("&&"))
REGISTER_BUILTIN(Orer, 2, 0, symbolFromStr("||"))
REGISTER_BUILTIN(Noter, 1, 0, symbolFromStr("not"))
REGISTER_BUILTIN(Ifer, 3, 0, symbolFromStr("if"))
REGISTER_BUILTIN(Elser, 1, 0, symbolFromStr("else"))
REGISTER_BUILTIN(Elifer, 1, 0, symbolFromStr("elif"))

void Logic_init(void) {
}

}; /* end namespace Evaluators */

