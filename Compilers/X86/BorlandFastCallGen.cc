#include "Compilers/X86/BorlandFastCallGen"

namespace Compilers {
namespace X86 {
BorlandFastCallGen::BorlandFastCallGen(CodeGen* code_gen) :
	PascalCallGen(code_gen)
{
}
void BorlandFastCallGen::build(AST::NodeT call_destination) {
	std::vector<AST::NodeT>::const_iterator end_iter = arguments.end();
	int index = 0;
	for(std::vector<AST::NodeT>::const_iterator iter = arguments.begin(); iter != end_iter; ++iter, ++index) {
		AST::NodeT argument = *iter;
		if(index == 0)
			code_gen->genLoadRegisterFromImmediate(AST::symbolFromStr("%eax"), argument);
		else if(index == 1)
			code_gen->genLoadRegisterFromImmediate(AST::symbolFromStr("%edx"), argument);
		else if(index == 2)
			code_gen->genLoadRegisterFromImmediate(AST::symbolFromStr("%ecx"), argument);
		else
			code_gen->genPush(argument);
	}
	code_gen->genCall(call_destination);
	code_gen->genStackThrowawayBits(stack_usage_afterwards);
}
}; /* end namespace */
}; /* end namespace */
