#ifdef WIN32
#include "stdafx.h"
#endif
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "Evaluators/Logic"
#include "Evaluators/Builtins"
#include "Evaluators/ModuleLoader"
#include "Values/Symbols"

namespace Evaluators {

using namespace Values;
#define fn makeAbstraction
#define call makeApplication
#define sy symbolFromStr
#define T sy("t")
#define F sy("f")
#define A sy("a")
#define B sy("b")
#define M sy("m")
#define R sy("r")
#define BER sy("ber")
#define WORLD sy("world")
/* TODO move ioValue, ioWorld from Runtime to here */
#define ioValue sy("ioValue")
#define ioWorld sy("ioWorld")
#define IO(entry) Evaluators::getModuleEntryAccessor("IO", entry)

//Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
NodeT aTrue = annotate(fn(T, fn(F, T)));
NodeT aFalse = annotate(fn(T, fn(F, F)));

NodeT internNative(bool value) {
	return(value ? aTrue : aFalse);
}

static NodeT sequence2(NodeT a, NodeT b, NodeT fallback) {
/*
let (;) := \m \ber \world
        let r := (m world) in
        ber (ioValue r) (ioWorld r)

which is

(\r ber (ioValue r) (ioWorld r)) (m world)
*/
	Evaluators::reduce(a);
	return(Evaluators::reduce(b));
}
NodeT Sequencer;

// \a \b \t \f a (b t f) f)
NodeT Ander = annotate(fn(A, fn(B, fn(T, fn(F, call(call(A, call(call(B, T), F)), F))))));

// \a \b \t \f a t (b t f)
NodeT Orer = annotate(fn(A, fn(B, fn(T, fn(F, call(call(A, T), call(call(B, T), F)))))));

// \a \t \f a f t
NodeT Noter = annotate(fn(A, fn(T, fn(F, call(call(A, F), T)))));

NodeT Ifer = annotate(fn(A, A));
NodeT Elser = annotate(fn(A, A)); /* actually \a \b a b */
NodeT Elifer = annotate(fn(A, A)); /* actually \a \b a b */

void initLogic(void) {
	Symbols::initSymbols();
	Sequencer = annotate(fn(M, fn(BER, fn(WORLD, 
	call(fn(R, call(call(BER, call(IO(ioValue), R)), call(IO(ioWorld), R))), 
		call(M, WORLD)))))); /* FIXME */
}

}; /* end namespace Evaluators */

