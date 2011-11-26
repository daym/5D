#include "FFIs/CallMarshaller"

namespace FFIs {

CallMarshaller::CallMarshaller(AST::Symbol* resultSignature, AST::Symbol* parameterSignature) {
	this->resultSignature = resultSignature;
	this->parameterSignature = parameterSignature; // plus the worldliness as the first char.
}
/* in order to get here, you do: (translateFFI 'i 'pii) 
   The next argument is the actual CProcedure, whose call will be wrapped in our abstractions. 
   Thus, for example int open(const char *pathname, int flags, mode_t mode); will be:
     \world \pathname \flags \mode (#LLopen pathname flags mode):world:nil
*/
AST::Node* CallMarshaller::executeLowlevel(AST::Node* argument) {
	return(NULL); /* FIXME */
}

};
