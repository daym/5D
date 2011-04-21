#include "FFIs/CallMarshaller"

namespace FFIs {

CallMarshaller::CallMarshaller(AST::Symbol* resultSignature, AST::Symbol* parameterSignature) {
	this->resultSignature = resultSignature;
	this->parameterSignature = parameterSignature;
}
AST::Node* CallMarshaller::executeLowlevel(AST::Node* argument) {
	return(NULL); /* FIXME */
}

};
