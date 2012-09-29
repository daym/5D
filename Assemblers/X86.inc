/* limitation in 64 bit mode: */

#define CHECK(v) {if(!v) return(0);}
#define OK return 1
#define MOD_REG (3 << 6)
#define MODCODE0(r1) r1
#define MODCODE1(r1) (MOD_REG | MODCODE0(r1))
#define MODCODE2(a,b) (MOD_REG | (MODCODE0(b) << 3) | MODCODE0(a))

/* returns: 1: ok */
int emitCode8(uint8_t code, FILE* f) {
	return (fputc(code, f) != EOF);
}
/* X86 is little endian */
static __inline int emitValue64(int64_t value, FILE* f) {
	int i;
	for(i = 0; i < 8; ++i) {
		CHECK(emitCode8(value&0xFF, f));
		value >>= 8;
	}
	OK;
}
static __inline int emitValue32(int32_t value, FILE* f) {
	int i;
	for(i = 0; i < 4; ++i) {
		CHECK(emitCode8(value&0xFF, f));
		value >>= 8;
	}
	OK;
}
/* FIXME */
#define FITS_SIGNED_32_BIT_P(v) 0
#define SILLY_FITS_SIGNED_32_BIT_P(v) (v >= -0x80000000L && v < 0x80000000L)

int emitLoadValReg(int64_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	if(REX_W && !FITS_SIGNED_32_BIT_P(value)) { /* 64 bit */
		CHECK(emitCode8(       dest == 0 ? 0xB8 : 
		                       dest == 1 ? 0xB9 : 
		                       dest == 2 ? 0xBA : 
		                       dest == 3 ? 0xBB : 
		                       dest == 4 ? 0xBC :
		                       dest == 5 ? 0xBD :
		                       dest == 6 ? 0xBE :
		                       dest == 7 ? 0xBF :
		                       0x00, f)); /* error */
		CHECK(emitValue64(value, f));
	} else {
		CHECK(emitCode8(0xC7, f)); /* mov $ */
		CHECK(emitCode8(MODCODE1(dest), f));
		CHECK(emitValue32(value&0xFFFFFFFF, f));
	}
	OK;
}
int emitLoadRegReg(int src, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x8B, f)); /* mov % */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	OK;
}
int emitPopReg(int dest, FILE* f) {
	/*implicit if(REX_W)
		CHECK(emitCode8(REX_W, f));*/
	CHECK(emitCode8(0x8F, f)); /* pop */
	CHECK(emitCode8(MODCODE1(dest), f));
	OK;
}
int emitPushReg(int src, FILE* f) {
	/*implicit if(REX_W)
		CHECK(emitCode8(REX_W, f));*/
	CHECK(emitCode8(0xFF, f)); /* PUSH % */
	CHECK(emitCode8(MODCODE1(src) | (6 << 3), f));
	OK;
}
int emitAddRegReg(int src, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x01, f)); /* ADD % */
	CHECK(emitCode8(MODCODE2(dest, src), f));
	OK;
}
int emitAddValReg(int32_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* add $ */
	CHECK(emitCode8(MODCODE1(dest), f));
	CHECK(emitValue32(value, f));
	OK;
}
int emitSubRegReg(int src, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x29, f)); /* sub % */
	CHECK(emitCode8(MODCODE2(dest, src), f));
	OK;
}
int emitSubValReg(int32_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* sub $ */
	CHECK(emitCode8(MODCODE1(dest) | (5 << 3), f));
	CHECK(emitValue32(value&0xFFFFFFFF, f));
	OK;
}
int emitMulRegReg(int src, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x0F, f)); /* imul % */
	CHECK(emitCode8(0xAF, f)); /* % */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	OK;
}
int emitMulValRegReg(int src, int32_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x69, f)); /* imul $ */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	CHECK(emitValue32(value, f));
	OK;
}
/* I hope that's a joke that I'm not getting... */
int emitIdivVal(int8_t divisor, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0xF7, f)); /* idiv $ */
	CHECK(emitCode8(MOD_REG | (7 << 3) | divisor, f));
	OK;
}
int emitRet(FILE* f) {
	CHECK(emitCode8(0xC3, f)); /* RET */
	OK;
}
int emitClearCarry(FILE* f) {
	CHECK(emitCode8(0xF8, f)); /* CLC */
	OK;
}
int emitSetCarry(FILE* f) {
	CHECK(emitCode8(0xF9, f)); /* STC */
	OK;
}
int emitAdc(int32_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* adc $ */
	CHECK(emitCode8(       dest == 0 ? 0xD0 : /* FIXME test */
	                       dest == 1 ? 0xD1 : 
	                       dest == 2 ? 0xD2 : 
	                       dest == 3 ? 0xD3 : 
	                       dest == 4 ? 0xD4 :
	                       dest == 5 ? 0xD5 :
	                       dest == 6 ? 0xD6 :
	                       dest == 7 ? 0xD7 :
	                       0x00, f)); /* error */
	CHECK(emitValue32(value, f));
	OK;
}
int emitSbb(int32_t value, int dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* sbb $ */
	CHECK(emitCode8(       dest == 0 ? 0xD8 : /* FIXME test */
	                       dest == 1 ? 0xD9 : 
	                       dest == 2 ? 0xDA : 
	                       dest == 3 ? 0xDB : 
	                       dest == 4 ? 0xDC :
	                       dest == 5 ? 0xDD :
	                       dest == 6 ? 0xDE :
	                       dest == 7 ? 0xDF :
	                       0x00, f)); /* error */
	CHECK(emitValue32(value, f));
	OK;
}
// FIXME push imm, pop imm.

static AST::NodeT regNames[] = { /* in hardware order! */
	SAX,
	SCX,
	SDX,
	SBX,
	SSP,
	SBP,
	SSI,
	SDI,
};

int emitPushRegs(uint16_t regs, FILE* f) {
	int i;
	for(i = 0; i < 8; ++i)
		if(regs & (1 << i))
			CHECK(emitPushReg(i, f));
	OK;
}
int emitPopRegs(uint16_t regs, FILE* f) {
	int i;
	for(i = 0; i < 8; ++i)
		if(regs & (1 << i))
			CHECK(emitPopReg(i, f));
	OK;
}

#include "Assemblers/Glue.inc"
int emit(AST::NodeT operator_, AST::NodeT a, AST::NodeT b, AST::NodeT c, FILE* f) {
	return (operator_ == SloadValReg) ? emitLoadValReg(I64(a), R(b), f) :
	       (operator_ == SloadRegReg) ? emitLoadRegReg(R(a), R(b), f) :
	       (operator_ == SpopReg) ? emitPopReg(R(a), f) :
	       (operator_ == SpushReg) ? emitPushReg(R(a), f) :
	       (operator_ == SpopRegs) ? emitPopRegs(I16(a), f) :
	       (operator_ == SpushRegs) ? emitPushRegs(I16(a), f) :
	       (operator_ == SaddRegReg) ? emitAddRegReg(R(a), R(b), f) :
	       (operator_ == SaddValReg) ? emitAddValReg(I32(a), R(b), f) : 
	       (operator_ == SsubRegReg) ? emitSubRegReg(R(a), R(b), f) : 
	       (operator_ == SsubValReg) ? emitSubValReg(I32(a), R(b), f) :
	       (operator_ == SmulRegReg) ? emitMulRegReg(R(a), R(b), f) : 
	       (operator_ == SmulValRegReg) ? emitMulValRegReg(R(a), I32(b), R(c), f) :
	       (operator_ == SidivVal) ? emitIdivVal(I8(a), f) /* WTF */ : 
	       (operator_ == Sret) ? emitRet(f) : 
	       (operator_ == SclearCarry) ? emitClearCarry(f) :
	       (operator_ == SsetCarry) ? emitSetCarry(f) : 
	       (operator_ == Sadc) ? emitAdc(I32(a), R(b), f) : 
	       (operator_ == Ssbb) ? emitSbb(I32(a), R(b), f) : 
		// FIXME exports:
		// FIXME register list.
	       0;
}
