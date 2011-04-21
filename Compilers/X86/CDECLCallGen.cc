#include "AST/AST"
#include "AST/Symbol"
#include "Compilers/X86/CDECLCallGen"
namespace Compilers {
namespace X86 {

void CDECLCallGen::build(AST::Node* call_destination) {
	std::vector<AST::Node*>::const_reverse_iterator end_iter = arguments.rend();
	for(std::vector<AST::Node*>::const_reverse_iterator iter = arguments.rbegin(); iter != end_iter; ++iter) {
		AST::Node* argument = *iter;
		code_gen->gen_push(argument);
		stack_usage_afterwards += code_gen->get_size_in_bits(argument);
	}
	code_gen->gen_call(call_destination);
	code_gen->gen_stack_throwaway_bits(stack_usage_afterwards);
}


}; /* end namespace */
}; /* end namespace */
