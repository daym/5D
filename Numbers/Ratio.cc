#include "Numbers/Ratio"
#include "Evaluators/Builtins"
namespace Numbers {
Ratio* makeRatio(AST::Node* aa, AST::Node* bb) {
	Ratio* result = new Ratio;
	result->a = aa;
	result->b = bb;
	return(result);
}
REGISTER_STR(Ratio, {
	std::stringstream result;
	result << '(' << node->a << '/' << node->b << ')';
	return(result.str());
})
}; /* end namespace Numbers */
