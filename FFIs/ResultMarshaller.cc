#include "FFIs/ResultMarshaller"
#include "FFIs/ArgumentMarshaller"

namespace FFIs {

AST::Node* ResultMarshaller::executeLowlevel(AST::Node* argument) /* argument is the result type signature. Result is an ArgumentMarshaller */
{
	return(new ArgumentMarshaller(dynamic_cast<AST::Symbol*>(argument)));
}
std::string ResultMarshaller::str(void) const {
	// FIXME the argument
	return("translateFFI");
}

};
