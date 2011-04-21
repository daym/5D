#include "Compilers/X86/BorlandFastCallGen"

namespace Compilers {
namespace X86 {

void BorlandFastCallGen::build(AST::Node* call_destination) {
	std::vector<AST::Node*>::const_iterator end_iter = arguments.end();
	int index = 0;
	for(std::vector<AST::Node*>::const_iterator iter = arguments.begin(); iter != end_iter; ++iter, ++index) {
		AST::Node* argument = *iter;
		if(index == 0)
			code_gen->gen_load_register(AST::intern("%eax"), argument);
		else if(index == 1)
			code_gen->gen_load_register(AST::intern("%edx"), argument);
		else if(index == 2)
			code_gen->gen_load_register(AST::intern("%ecx"), argument);
		else
			code_gen->gen_push(argument);
	}
	code_gen->gen_call(call_destination);
	code_gen->gen_stack_throwaway_bits(stack_usage_afterwards);
}
}; /* end namespace */
}; /* end namespace */
