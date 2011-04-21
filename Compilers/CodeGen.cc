#include "Compilers/CodeGen"
namespace Compilers {
unsigned CodeGen::get_size_in_bits(AST::Node* source) {
	/* FIXME */
	return(42);
}
void CodeGen::gen_push(AST::Node* source) {
	/* FIXME push source */
}
void CodeGen::gen_stack_trowaway_bits(int bit_count) {
	/* FIXME add bit_count / 8, %esp */
}
void CodeGen::gen_call(AST::Node* destination) {
	/* FIXME call destination */
}
}; /* end namespace Compilers */
