#include <stdlib.h>
#include <assert.h>
#include "Values/Values"
#include "Numbers/Ratio"
#include "Evaluators/Builtins"
namespace Numbers {
using namespace Values;
Ratio* makeRatio(NodeT aa, NodeT bb) {
	Ratio* result = new Ratio;
	result->a = aa;
	result->b = bb;
	return(result);
}
bool ratio_P(NodeT n) {
	return(dynamic_cast<Ratio*>(n) != NULL);
}
static NodeT makeRatioB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	NodeT a = iter->second;
	++iter;
	NodeT b = iter->second;
	//++iter;
	return(makeRatio(a, b));
}
static inline NodeT ensureRatio(NodeT node) {
	if(!ratio_P(node))
		throw EvaluationException("argument is not a Ratio");
	return(node);
}
DEFINE_FULL_OPERATION(RatioMaker, return(makeRatioB(fn, argument)))
DEFINE_SIMPLE_OPERATION(RatioP, (dynamic_cast<Ratio*>(reduce(argument)) !=NULL||dynamic_cast<Ratio*>(reduce(argument)) != NULL))
DEFINE_SIMPLE_OPERATION(RatioNumeratorGetter, Ratio_getA(ensureRatio(reduce(argument))))
DEFINE_SIMPLE_OPERATION(RatioDenominatorGetter, Ratio_getB(ensureRatio(reduce(argument))))

/*
a/b + c/d = (a*d + c*b)/(b*d)
a/b - c/d = (a*d - c*b)/(b*d)
a/b ⋅ c/d = a*c/(b*d)       
a/b / c/d = a/b ⋅ d/c = a*d/(b*c)
*/
REGISTER_STR(Ratio, {
	std::stringstream result;
	result << '(' << Evaluators::str(node->a) << "/" << Evaluators::str(node->b) << ')';
	return(result.str());
})
REGISTER_BUILTIN(RatioMaker, 2, 0, symbolFromStr("makeRatio"))
REGISTER_BUILTIN(RatioP, 1, 0, symbolFromStr("ratio?"))
REGISTER_BUILTIN(RatioNumeratorGetter, 1, 0, symbolFromStr("ratioNum"))
REGISTER_BUILTIN(RatioDenominatorGetter, 1, 0, symbolFromStr("ratioDenom"))
}; /* end namespace Numbers */
