/* limitation in 64 bit mode: */

#define CHECK(v) {if(!v) return(0);}
#define OK return 1
#define MOD_REG (3 << 6)
#define MODCODE0(r1) (r1 == SAX ? 0 : \
		                       r1 == SCX ? 1 : \
		                       r1 == SDX ? 2 : \
		                       r1 == SBX ? 3 : \
		                       r1 == SSP ? 4 :\
		                       r1 == SBP ? 5 :\
		                       r1 == SSI ? 6 : \
		                       r1 == SDI ? 7 : \
		                       0x00)
#define MODCODE1(r1) (MOD_REG | MODCODE0(r1))
#define MODCODE2(a,b) (MOD_REG | (MODCODE0(b) << 3) | MODCODE0(a))

/* returns: 1: ok */
int emitCode8(uint8_t code, FILE* f) {
	return (fputc(code, f) != EOF);
	/* FIXME error handling */
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

int emitLoadRegImm(int64_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	if(REX_W && !FITS_SIGNED_32_BIT_P(value)) { /* 64 bit */
		CHECK(emitCode8(       dest == SAX ? 0xB8 : 
		                       dest == SCX ? 0xB9 : 
		                       dest == SDX ? 0xBA : 
		                       dest == SBX ? 0xBB : 
		                       dest == SSP ? 0xBC :
		                       dest == SBP ? 0xBD :
		                       dest == SSI ? 0xBE :
		                       dest == SDI ? 0xBF :
		                       0x00, f)); /* error */
		CHECK(emitValue64(value, f));
	} else {
		CHECK(emitCode8(0xC7, f)); /* mov $ */
		CHECK(emitCode8(MODCODE1(dest), f));
		CHECK(emitValue32(value&0xFFFFFFFF, f));
	}
	OK;
}
int emitLoadRegReg(AST::NodeT src, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x8B, f)); /* mov % */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	OK;
}


int emitPop(AST::NodeT dest, FILE* f) {
	/*implicit if(REX_W)
		CHECK(emitCode8(REX_W, f));*/
	CHECK(emitCode8(0x8F, f)); /* pop */
	CHECK(emitCode8(MODCODE1(dest), f));
	OK;
}
int emitPush(AST::NodeT src, FILE* f) {
	/*implicit if(REX_W)
		CHECK(emitCode8(REX_W, f));*/
	CHECK(emitCode8(0xFF, f)); /* PUSH % */
	CHECK(emitCode8(MODCODE1(src) | (6 << 3), f));
	OK;
}
int emitAddRegReg(AST::NodeT src, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x01, f)); /* ADD % */
	CHECK(emitCode8(MODCODE2(dest, src), f));
	OK;
}
int emitAddValReg(int32_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* add $ */
	CHECK(emitCode8(MODCODE1(dest), f));
	CHECK(emitValue32(value, f));
	OK;
}
int emitSubRegReg(AST::NodeT src, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x29, f)); /* sub % */
	CHECK(emitCode8(MODCODE2(dest, src), f));
	OK;
}
int emitSubValReg(int32_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* sub $ */
	CHECK(emitCode8(MODCODE1(dest) | (5 << 3), f));
	CHECK(emitValue32(value&0xFFFFFFFF, f));
	OK;
}
int emitMulRegReg(AST::NodeT src, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x0F, f)); /* imul % */
	CHECK(emitCode8(0xAF, f)); /* % */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	OK;
}
int emitMulValRegReg(AST::NodeT src, int32_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x69, f)); /* imul $ */
	CHECK(emitCode8(MODCODE2(src, dest), f));
	CHECK(emitValue32(value, f));
	OK;
}
/* I hope that's a joke that I'm not getting... */
int emitIdivImm(int8_t divisor, FILE* f) {
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
int emitAdc(int32_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* adc $ */
	CHECK(emitCode8(       dest == SAX ? 0xD0 : /* FIXME test */
	                       dest == SCX ? 0xD1 : 
	                       dest == SDX ? 0xD2 : 
	                       dest == SBX ? 0xD3 : 
	                       dest == SSP ? 0xD4 :
	                       dest == SBP ? 0xD5 :
	                       dest == SSI ? 0xD6 :
	                       dest == SDI ? 0xD7 :
	                       0x00, f)); /* error */
	CHECK(emitValue32(value, f));
	OK;
}
int emitSbb(int32_t value, AST::NodeT dest, FILE* f) {
	if(REX_W)
		CHECK(emitCode8(REX_W, f));
	CHECK(emitCode8(0x81, f)); /* sbb $ */
	CHECK(emitCode8(       dest == SAX ? 0xD8 : /* FIXME test */
	                       dest == SCX ? 0xD9 : 
	                       dest == SDX ? 0xDA : 
	                       dest == SBX ? 0xDB : 
	                       dest == SSP ? 0xDC :
	                       dest == SBP ? 0xDD :
	                       dest == SSI ? 0xDE :
	                       dest == SDI ? 0xDF :
	                       0x00, f)); /* error */
	CHECK(emitValue32(value, f));
	OK;
}
#include "Assemblers/Glue.inc"
int emit(AST::NodeT operator_, AST::NodeT a, AST::NodeT b, AST::NodeT c, FILE* f) {
	return (operator_ == SloadRegImm) ? emitLoadRegImm(I64(a), R(b), f) :
	       (operator_ == SloadRegReg) ? emitLoadRegReg(R(a), R(b), f) :
	       (operator_ == Spop) ? emitPop(R(a), f) :
	       (operator_ == Spush) ? emitPush(R(a), f) :
	       (operator_ == SaddRegReg) ? emitAddRegReg(R(a), R(b), f) :
	       (operator_ == SaddValReg) ? emitAddValReg(I32(a), R(b), f) : 
	       (operator_ == SsubRegReg) ? emitSubRegReg(R(a), R(b), f) : 
	       (operator_ == SsubValReg) ? emitSubValReg(I32(a), R(b), f) :
	       (operator_ == SmulRegReg) ? emitMulRegReg(R(a), R(b), f) : 
	       (operator_ == SmulValRegReg) ? emitMulValRegReg(R(a), I32(b), R(c), f) :
	       (operator_ == SidivImm) ? emitIdivImm(I8(a), f) /* WTF */ : 
	       (operator_ == Sret) ? emitRet(f) : 
	       (operator_ == SclearCarry) ? emitClearCarry(f) :
	       (operator_ == SsetCarry) ? emitSetCarry(f) : 
	       (operator_ == Sadc) ? emitAdc(I32(a), R(b), f) : 
	       (operator_ == Ssbb) ? emitSbb(I32(a), R(b), f) :
	       0;
}
