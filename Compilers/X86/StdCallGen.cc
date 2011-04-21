#include "AST/AST"
#include "AST/Symbol"
#include "Compilers/X86/StdCallGen"
namespace Compilers {
namespace X86 {

void StdCallGen::build(AST::Node* call_destination) {
	std::vector<AST::Node*>::const_reverse_iterator end_iter = arguments.rend();
	for(std::vector<AST::Node*>::const_reverse_iterator iter = arguments.rbegin(); iter != end_iter; ++iter)
		code_gen->gen_push(*iter);
	code_gen->gen_call(call_destination);
	/* callee cleans its mess up! */
	code_gen->gen_stack_throwaway_bits(stack_usage_afterwards);
}


}; /* end namespace */
}; /* end namespace */
