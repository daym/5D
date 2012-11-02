#include "Values/Values"
#include "Compilers/X86/StdCallGen"
namespace Compilers {
namespace X86 {
using namespace Values;
StdCallGen::StdCallGen(CodeGen* code_gen) :
	Compilers::CallGen(code_gen)
{
}
void StdCallGen::build(NodeT call_destination) {
	std::vector<NodeT>::const_reverse_iterator end_iter = arguments.rend();
	for(std::vector<NodeT>::const_reverse_iterator iter = arguments.rbegin(); iter != end_iter; ++iter)
		code_gen->genPush(*iter);
	code_gen->genCall(call_destination);
	/* callee cleans its mess up! */
	code_gen->genStackThrowawayBits(stack_usage_afterwards);
}


}; /* end namespace */
}; /* end namespace */
