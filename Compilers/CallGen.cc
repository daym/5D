#include "Compilers/CallGen"

namespace Compilers {

CallGen::CallGen(CodeGen* code_gen) {
		this->code_gen = code_gen;
		clear();
}
void CallGen::clear(void) {
		arguments.clear();
		stack_usage_afterwards = 0;
}
void CallGen::build(AST::Node* call_destination) {
		//[code_gen->gen_push(source) for source in arguments];
		code_gen->gen_call(call_destination);
		code_gen->gen_stack_throwaway_bits(stack_usage_afterwards);
}

}; /* end namespace Compilers */
