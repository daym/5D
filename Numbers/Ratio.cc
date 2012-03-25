#include <stdlib.h>
#include <assert.h>
#include "Numbers/Ratio"
#include "Evaluators/Builtins"
namespace Numbers {
Ratio* makeRatio(AST::Node* aa, AST::Node* bb) {
	Ratio* result = new Ratio;
	result->a = aa;
	result->b = bb;
	return(result);
}
bool ratio_P(AST::Node* n) {
	return(dynamic_cast<Ratio*>(n) != NULL);
}
static AST::Node* makeRatioB(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::Node* a = iter->second;
	++iter;
	AST::Node* b = iter->second;
	//++iter;
	return(makeRatio(a, b));
}
DEFINE_FULL_OPERATION(RatioMaker, return(makeRatioB(fn, argument)))
DEFINE_SIMPLE_OPERATION(RatioP, (dynamic_cast<Ratio*>(reduce(argument)) !=NULL||dynamic_cast<Ratio*>(reduce(argument)) != NULL))
DEFINE_SIMPLE_OPERATION(RatioNumeratorGetter, Ratio_getA(reduce(argument)))
DEFINE_SIMPLE_OPERATION(RatioDenominatorGetter, Ratio_getB(reduce(argument)))

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
REGISTER_BUILTIN(RatioMaker, 2, 0, AST::symbolFromStr("makeRatio"))
REGISTER_BUILTIN(RatioP, 1, 0, AST::symbolFromStr("ratio?"))
REGISTER_BUILTIN(RatioNumeratorGetter, 1, 0, AST::symbolFromStr("ratioNum"))
REGISTER_BUILTIN(RatioDenominatorGetter, 1, 0, AST::symbolFromStr("ratioDenom"))
}; /* end namespace Numbers */
