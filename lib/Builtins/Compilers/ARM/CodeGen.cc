#include "Compilers/ARM/CodeGen"
namespace Compilers {
namespace ARM {
unsigned CodeGen::get_size_in_bits(AST::NodeT source) {
        /* TODO actually inspect source and return a decent size (however that would work...) */
        return(32);
}
void CodeGen::gen_push(AST::NodeT source) {
        /* FIXME push source */
}
void CodeGen::gen_stack_throwaway_bits(int bit_count) {
        /* FIXME add bit_count / 8, %esp */
}
void CodeGen::gen_call(AST::NodeT destination) {
        /* FIXME call destination */
}
void CodeGen::gen_load_register(AST::Symbol* register_, AST::NodeT value) {
        /* FIXME load register with value */
}
void CodeGen::gen_enter_frame(void) {
	/* FIXME stm %lr */
}
void CodeGen::gen_leave_frame(void) {
	/* FIXME ldm %lr */
}
void CodeGen::gen_return(int bit_count) {
	/* FIXME mov %lr, %pc */
}

}; /* end namespace ARM */
}; /* end namespace Compilers */
