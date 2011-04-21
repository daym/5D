#include "AST/AST"
#include "FFIs/ArgumentMarshaller"
#include "FFIs/CallMarshaller"

namespace FFIs {

ArgumentMarshaller::ArgumentMarshaller(AST::Symbol* resultSignature) {
	this->resultSignature = resultSignature;
}
AST::Node* ArgumentMarshaller::executeLowlevel(AST::Node* argument) {
	return(new CallMarshaller(resultSignature, dynamic_cast<AST::Symbol*>(argument)));
}

};
