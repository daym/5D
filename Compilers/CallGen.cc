#include "Compilers/CallGen"

namespace Compilers {

CallGen::CallGen(CodeGen* code_gen) {
		this->code_gen = code_gen;
		clear();
}
void CallGen::clear(void) {
		argument_count = 0;
		stack_usage_afterwards = 0;
}
void CallGen::add(AST::Node* source) {
		header.push_back(code_gen->gen_push(source));
		stack_usage_afterwards += code_gen->get_size_in_bits(source);
		argument_count += 1;
}
void CallGen::build(AST::Node* call_destination) {
		emit header;
		code_gen->gen_call(call_destination);
		code_gen->gen_stack_trowaway_bits(stack_usage_afterwards);
}

}; /* end namespace Compilers */
