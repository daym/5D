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
void CallGen::build(Values::NodeT call_destination) {
		//[code_gen->gen_push(source) for source in arguments];
		code_gen->genCall(call_destination);
		code_gen->genStackThrowawayBits(stack_usage_afterwards);
}

}; /* end namespace Compilers */
