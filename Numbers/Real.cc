#include "Numbers/Real"
#include "Evaluators/Builtins"

namespace Evaluators {
AST::Node* internNative(bool value);
};

namespace Numbers {

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
Real* operator+(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator-(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator*(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}

}; /* end namespace Numbers */
