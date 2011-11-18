#include "Numbers/Real"

namespace Numbers {

AST::Node* internNative(NativeFloat value) {
	return(new Float(value));
}

AST::Node* FloatP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Float*>(argument) != NULL;
	return(internNative(result));
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
