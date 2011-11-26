#include "FFIs/ResultMarshaller"
#include "FFIs/ArgumentMarshaller"
#include "Evaluators/Builtins"

namespace FFIs {

AST::Node* ResultMarshaller::executeLowlevel(AST::Node* argument) /* argument is the result type signature. Result is an ArgumentMarshaller */
{
	return(new ArgumentMarshaller(dynamic_cast<AST::Symbol*>(argument)));
}
REGISTER_STR(ResultMarshaller, return("translateFFI");)

};
