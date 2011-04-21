#include "Compilers/X86/PascalCallGen"
namespace Compilers {
namespace X86 {

void PascalCallGen::build(AST::Node* call_destination) {
	std::vector<AST::Node*>::const_iterator end_iter = arguments.end();
	for(std::vector<AST::Node*>::const_iterator iter = arguments.begin(); iter != end_iter; ++iter) {
		AST::Node* argument = *iter;
		code_gen->gen_push(argument);
	}
	code_gen->gen_call(call_destination);
	code_gen->gen_stack_throwaway_bits(stack_usage_afterwards);
}

};
};
