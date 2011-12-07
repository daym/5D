#include <string.h>
#include "Numbers/Real"
#include "Evaluators/Builtins"

namespace Evaluators {
AST::Node* internNative(bool value);
};

namespace Numbers {

REGISTER_STR(Float, {
	std::stringstream sst;
	sst.precision(7); /* 17 for some */
	sst << node->value;
	std::string v = sst.str();
	const char* vc = v.c_str();
	if(strpbrk(vc, ".eE") == NULL)
		sst << ".0";
	return(sst.str());
})

REGISTER_STR(Real, return("FIXME");)

AST::Node* internNative(NativeFloat value) {
	return(new Float(value));
}

DEFINE_SIMPLE_OPERATION(FloatP, dynamic_cast<Float*>(reduce(argument)) != NULL)

AST::Node* operator+(const Float& a, const Float& b) {
	return(internNative(a.value + b.value)); /* FIXME */
}
AST::Node* operator-(const Float& a, const Float& b) {
	return(internNative(a.value - b.value)); /* FIXME */
}
AST::Node* operator*(const Float& a, const Float& b) {
	return(internNative(a.value * b.value)); /* FIXME */
}
AST::Node* operator/(const Float& a, const Float& b) {
	return(internNative(a.value / b.value)); /* FIXME */
}
Real* operator+(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator-(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator*(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator/(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
AST::Node* operator<=(const Float& a, const Float& b) {
	return(Evaluators::internNative(a.value <= b.value));
}
AST::Node* operator<=(const Real& a, const Real& b) {
	return(Evaluators::internNative(false)); /* FIXME */
}

REGISTER_BUILTIN(FloatP, 1, 0, AST::symbolFromStr("float?"))

NativeFloat toNativeFloat(AST::Node* node, bool& B_ok) {
	Float* floatNode;
	Real* realNode;
	B_ok = false;
	node = evaluate(node);
	if(node == NULL)
		return(0);
	else if((floatNode = dynamic_cast<Float*>(node)) != NULL) {
		B_ok = true;
		return(floatNode->value);
	} else if((realNode = dynamic_cast<Real*>(node)) != NULL) {
		//NativeFloat result = realNode->toNativeFloat();
		//B_ok = true;
		return(0.0f);
	} else
		return(0.0f);
}

}; /* end namespace Numbers */
