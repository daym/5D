#include "Compilers/X86/PascalCallGen"
namespace Compilers {
namespace X86 {

PascalCallGen::PascalCallGen(CodeGen* code_gen) :
	CallGen(code_gen)
{
}
void PascalCallGen::build(AST::NodeT call_destination) {
	std::vector<AST::NodeT>::const_iterator end_iter = arguments.end();
	for(std::vector<AST::NodeT>::const_iterator iter = arguments.begin(); iter != end_iter; ++iter) {
		AST::NodeT argument = *iter;
		code_gen->genPush(argument);
	}
	code_gen->genCall(call_destination);
	code_gen->genStackThrowawayBits(stack_usage_afterwards);
}

};
};
