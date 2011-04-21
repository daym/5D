#include "Compilers/X86/CodeGen"
namespace Compilers {
namespace X86 {
unsigned CodeGen::get_size_in_bits(AST::Node* source) {
        /* FIXME */
        return(32);
}
void CodeGen::gen_push(AST::Node* source) {
        /* if source is a symbol, we assume it means a register. */
        AST::Symbol* sourceSymbol = dynamic_cast<AST::Symbol*>(source);
		if(sourceSystem) {
			if(sourceSystem == AST::intern("%eax")) {
				//0x50+r
			}
		}
}
void CodeGen::gen_pop(AST::Node* source) {
        /* if source is a symbol, we assume it means a register. */
        AST::Symbol* sourceSymbol = dynamic_cast<AST::Symbol*>(source);
		if(sourceSystem) {
			if(sourceSystem == AST::intern("%eax")) {
				//0x58+r
			}
		}
}
void CodeGen::gen_stack_throwaway_bits(int bit_count) {
        /* FIXME add bit_count / 8, %esp */
}
void CodeGen::gen_call(AST::Node* destination) {
        /* FIXME call destination */
}
void CodeGen::gen_load_register(AST::Symbol* register_, AST::Node* value) {
        /* FIXME load register with value */
}
void CodeGen::gen_enter_frame(void) {
	/* FIXME push %ebp
	         mov %esp, %ebp */
}
void CodeGen::gen_leave_frame(void) {
	/* FIXME pop %ebp */
}
void CodeGen::gen_return(int bit_count) {
	/* FIXME ret */
}

}; /* end namespace X86 */
}; /* end namespace Compilers */
