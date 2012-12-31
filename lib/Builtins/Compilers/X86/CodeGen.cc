#include "Compilers/X86/CodeGen"
namespace Compilers {
namespace X86 {
unsigned CodeGen::get_size_in_bits(AST::NodeT source) {
        /* FIXME */
        return(32);
}
void CodeGen::gen_push(AST::NodeT source) {
        /* if source is a symbol, we assume it means a register. */
        AST::Symbol* sourceSymbol = dynamic_cast<AST::Symbol*>(source);
		if(sourceSymbol) {
			if(sourceSymbol == AST::symbolFromStr("%eax")) {
				//0x50+r
/*
*    push eax  50
*    push ebx  53
*    push ecx  51
*    push edx  52
*    push edi  57
*    push esi  56
*    push esp  54
*    push ebp  55
*    push [eax]	FF 30
*    push [ebx]	FF 33
*    push [ecx]	FF 31
*    push [edx]	FF 32
*    push [edi]	FF 37
*    push [esi]	FF 36
*    push [esp]	FF 34 24
*    push [ebp]	FF 75 00
*/
			}
		}
}
void CodeGen::gen_pop(AST::NodeT source) {
        /* if source is a symbol, we assume it means a register. */
        AST::Symbol* sourceSymbol = dynamic_cast<AST::Symbol*>(source);
		if(sourceSymbol) {
			if(sourceSymbol == AST::symbolFromStr("%eax")) {
				//0x58+r
/*
*    pop eax  58
*    pop ebx  5B
*    pop ecx  59
*    pop edx  5A
*    pop edi  5F
*    pop esi  5E
*    pop esp  5C
*    pop ebp  5D
*/
			}
		}
}
void CodeGen::gen_stack_throwaway_bits(int bit_count) {
        /* FIXME add bit_count / 8, %esp */
}
void CodeGen::gen_call(AST::NodeT destination) {
        /* FIXME call destination */
	/*
*    call eax  FF D0
*    call ecx  FF D1
*    call edx  FF D2  
*    call ebx  FF D3
*    call edi  FF D7
*    call esi  FF D6
*    call esp  FF D4
*    call ebp  FF D5
*    call [eax]	FF 10
*    call [ecx]	FF 11
*    call [edx]	FF 12
*    call [ebx]	FF 13
*    call [edi]	FF 17
*    call [esi]	FF 16
*    call [esp]	FF 14 24
*    call [ebp]   FF 55 00
*    jmp eax  FF E0
*    jmp ecx  FF E1
*    jmp edx  FF E2
*    jmp ebx  FF E3
*    jmp edi  FF E7
*    jmp esi  FF E6
*    jmp esp  FF E4
*    jmp ebp  FF E5
*
*    jmp [eax]	FF 20
*    jmp [ecx]	FF 21
*    jmp [edx]	FF 22
*    jmp [ebx]	FF 23
*    jmp [edi]	FF 27
*    jmp [esi]	FF 26
*    jmp [esp]	FF 24 24
*    jmp [ebp]	FF 65 00
*

	*/
}
void CodeGen::gen_load_register(AST::Symbol* register_, AST::NodeT value) {
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
	/* FIXME 0xC3 */
}

}; /* end namespace X86 */
}; /* end namespace Compilers */
