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
	return(sst.str());
})

REGISTER_STR(Real, return("FIXME");)

AST::Node* internNative(NativeFloat value) {
	return(new Float(value));
}

AST::Node* FloatP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Float*>(argument) != NULL;
	return(Evaluators::internNative(result));
}

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

}; /* end namespace Numbers */
