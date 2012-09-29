

/* all opcodes start with "e"... what does that mean? */
#define MR(r1) (r1 == SR0 ? 0 : \
		                       r1 == SR1 ? 1 : \
		                       r1 == SR2 ? 2 : \
		                       r1 == SR3 ? 3 : \
		                       r1 == SR4 ? 4 :\
		                       r1 == SR5 ? 5 :\
		                       r1 == SR6 ? 6 : \
		                       r1 == SR7 ? 7 : \
		                       r1 == SR8 ? 8 : \
		                       r1 == SR9 ? 9 : \
		                       r1 == SR10 ? 10 : \
		                       r1 == SR11 ? 11 :\
		                       r1 == SR12 ? 12 :\
		                       r1 == SR13 ? 13 : \
		                       r1 == SR14 ? 14 : \
		                       r1 == SR15 ? 15 : \
		                       0x00)

/* c is destination */
int emitInstruction(uint16_t opcode, int4_t a, int4_t b, int4_t c, FILE* f) {
	CHECK(emitCode8(opcode >> 4, f));
	CHECK(emitCode8(((opcode&0xF) << 4) | b, f));
	CHECK(emitCode8(a << 4, f)); // and other things.
	CHECK(emitCode8(c, f)); /* just above C here are 2 nibbles for other purposes. */
	OK;
}
int emitMInstruction(uint16_t opcode, int4_t mode, uint16_t flags, FILE* f) {
	CHECK(emitCode8(opcode >> 4, f));
	CHECK(emitCode8(((opcode&0xF) << 4) | mode, f));
	CHECK(emitCode8(flags >> 8, f));
	CHECK(emitCode8(flags&0xFF, f));
	OK;
}

int emitAddRegRegReg(AST::NodeT a, AST::NodeT b, AST::NodeT c, FILE* f) {
	return emitInstruction(0xE08, MR(a), MR(b), MR(c), f);
}
int emitSubRegRegReg(AST::NodeT a, AST::NodeT b, AST::NodeT c, FILE* f) {
	return emitInstruction(0xE04, MR(a), MR(b), MR(c), f);
}
int emitLoadRegReg(AST::NodeT src, AST::NodeT dest) {
	return emitInstruction(0xE1A, MR(src), MR(dest), 0, f);
}
/* FIXME bigger values */
int emitLoadRegImm(int8_t value, AST::NodeT dest) {
	return emitInstruction(0xE3A, MR(dest), 0, value, f);
}
int emitMulRegRegReg(AST::NodeT a, AST::NodeT b, AST::NodeT c, FILE* f) {
	return emitInstruction(0xE04, MR(a), MR(b), MR(c), f);
}
int emitPushRegs(uint16_t regs, FILE* f) {
	return emitMInstruction(0xE92, 0xD, regs, f);
}
int emitPopRegs(uint16_t regs, FILE* f) {
	return emitMInstruction(0xE8B, 0xD, regs, f);
}
static AST::NodeT regNames[] = {
	SR0,
	SR1,
	SR2,
	SR3,
	SR4,
	SR5,
	SR6,
	SR7,
	SR8,
	SR9,
	SR10,
	SR11,
	SR12,
	SR13,
	SR14,
	SR15,
};
int emitPushReg(AST::NodeT reg, FILE* f) {
	regNames.index(reg)
}
int emitPopReg(AST::NodeT reg, FILE* f) {
	regNames.index(reg)
}
/*
   4:	e1a0f00e 	mov	pc, lr
  3c:	e080000f 	add	r0, r0, pc
        ^^^^^  ^
        INSba  c
  80:	e3a0000a 	mov	r0, #10	; 0xa
  84:	e3a0100a 	mov	r1, #10	; 0xa
  7c:	e00a0f91 	mul	sl, r1, pc
  7c:	e041a00f 	sub	sl, r1, pc
  40:	e0811000 	add	r1, r1, r0
  80:	e92d0010 	push	{r4}
  84:	e8bd0010 	pop	{r4}
  80:	e92d0013 	push	{r0, r1, r4}
  80:	e92d0033 	push	{r0, r1, r4, r5}
  80:	e92d8033 	push	{r0, r1, r4, r5, pc}
            ^
  84:	e8bd0013 	pop	{r0, r1, r4}
  84:	e8bd0033 	pop	{r0, r1, r4, r5}
  84:	e8bd8033 	pop	{r0, r1, r4, r5, pc}

*/
/* TODO idiv push pop
               (operator_ == Sret) ? emitRet(f) : 
               (operator_ == SclearCarry) ? emitClearCarry(f) :
               (operator_ == SsetCarry) ? emitSetCarry(f) : 
               (operator_ == Sadc) ? emitAdc(I32(a), R(b), f) : 
               (operator_ == Ssbb) ? emitSbb(I32(a), R(b), f)

*/
