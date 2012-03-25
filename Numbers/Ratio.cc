#include <stdlib.h>
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
}; /* end namespace Numbers */
