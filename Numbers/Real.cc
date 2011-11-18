#include "Numbers/Real"

namespace Numbers {

AST::Node* internNative(NativeFloat value) {
	return(new Float(value));
}

AST::Node* FloatP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Float*>(argument) != NULL;
	return(internNative(result));
}

}; /* end namespace Numbers */
