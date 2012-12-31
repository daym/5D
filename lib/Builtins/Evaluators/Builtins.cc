#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <functional>
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Evaluators/FFI"
#include "Scanners/MathParser"
#include "FFIs/FFIs"
#include "Numbers/Integer"
#include <5D/Operations>
#include <5D/Allocators>
#include <5D/FFIs>
#include "Formatters/Math"
#include "Formatters/SExpression"
#include "Numbers/Real"
#include "Numbers/Ratio"

namespace Evaluators {

using namespace Values;
//NodeT churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
NodeT internNative(const char* value) {
	return(makeStr(value));
}

using namespace Numbers;
static BigUnsigned unsigneds[] = {
	BigUnsigned(0),
	BigUnsigned(1),
	BigUnsigned(2),
	BigUnsigned(3),
	BigUnsigned(4),
	BigUnsigned(5),
	BigUnsigned(6),
	BigUnsigned(7),
	BigUnsigned(8),
	BigUnsigned(9),
	BigUnsigned(10),
	BigUnsigned(11),
	BigUnsigned(12),
	BigUnsigned(13),
	BigUnsigned(14),
	BigUnsigned(15),
	BigUnsigned(16),
};
static std::map<Symbol*, NodeT> cachedDynamicBuiltins;
static NodeT getDynamicBuiltinC(const char* name) {
	if((name[0] >= '0' && name[0] <= '9') || name[0] == '-') {
		long int value;
		char* endptr = NULL;
		value = strtol(name, &endptr, 10);
		if(endptr && ((*endptr) == 'E' || (*endptr) == 'e')) { /* either a real or an integer specified in a weird way */
			// TODO maybe check: exponent >= number of fractional digits, then it's Integer after all...
		}
		if(endptr && *endptr) { /* maybe a real */
			NativeFloat value;
			std::istringstream sst(name);
			if(sst >> value)
				return(Numbers::internNative(value));
			else
				return(NULL);
		} else { /* maybe too big anyway */
			if(value == LONG_MIN || value == LONG_MAX) {
				const char* nrString;
				bool B_negative = false;
				if(name[0] == '-') {
					nrString = name + 1;
					B_negative = true;
				} else {
					nrString = name;
				}
				BigUnsigned v;
				bool B_zero = true;
				for(; *nrString; ++nrString) {
					char c = (*nrString);
					if(c < '0' || c > '9')
						return(NULL);
					if(c != '0')
						B_zero = false;
					v = v * unsigneds[10] + unsigneds[c - '0'];
				}
				return(new Integer(v, B_zero ? Integer::zero : B_negative ? Integer::negative : Integer::positive));
			}
		}
		return(Numbers::internNative((NativeInt) value));
	}/* else if(name[0] == 'i') {
		if(strcmp(name, "inf.0") == 0)
			return(Numbers::internNative((Numbers::NativeFloat) 1.0 / 0.0));
	} else if(name[0] == 'n') {
		if(strcmp(name, "nan.0") == 0)
			return(Numbers::internNative((Numbers::NativeFloat) 0.0 / 0.0));
	}*/
	return(NULL);
}
static NodeT getDynamicBuiltinA(NodeT sym) {
	if(symbol_P(sym))
		return(getDynamicBuiltinC(get_symbol_name(sym)));
	else
		return(NULL);
}
NodeT provide_dynamic_builtins_impl(NodeT body, HashTable::const_iterator end_iter, HashTable::const_iterator iter) {
	if(iter == end_iter)
		return(body);
	else {
		const char* name = iter->first;
		NodeT value = getDynamicBuiltinC(name); // TODO use passed getter
		++iter;
		return(value ? Evaluators::close(symbolFromStr(name), value, provide_dynamic_builtins_impl(body, end_iter, iter)) : provide_dynamic_builtins_impl(body, end_iter, iter));
	}
}
NodeT provide_dynamic_builtins(NodeT body) {
	HashTable freeNames;
	getFreeVariables(body, freeNames);
	HashTable::const_iterator endIter = freeNames.end();
	return(provide_dynamic_builtins_impl(body, endIter, freeNames.begin()));
}

Float promoteToFloat(const Int& v) {
	return(Float((NativeFloat) v.value));
}
Float promoteToFloat(const Integer& v) {
	float result = 0.0f;
	float sign = (v.getSign() != Integer::negative) ? 1.0f : (-1.0f);
	BigUnsigned q(v.getMagnitude());
	BigUnsigned divisor(10);
	for(int i = 0; i < 10000; ++i) {
		if(q.isZero())
			break;
		BigUnsigned r;
		r = q;
		r.divideWithRemainder(divisor, q);
		NativeInt rf = r.convertToSignedPrimitive<NativeInt>();
		result = result * 10.0f + rf;
	}
	return(Float(result * sign));
}
static Integer* toHeap(const Integer& v) {
	return(new Integer(v));
}
static NodeT toHeap(NodeT v) {
	return(v);
}
static Int* toHeap(const Int& v) {
	return(new Int(v));
}
static Float* toHeap(const Float& v) {
	return(new Float(v));
}
static Real* toHeap(const Real& v) {
	return(new Real(v));
}
static NodeT toHeap(bool v) {
	return(internNative(v));
}

static NodeT divmod0Int(const Numbers::Int& a, const Numbers::Int& b) {
	if(b.value == 0)
		throw EvaluationException("division by zero");
	NativeInt q = a.value / b.value;
	NativeInt r = a.value % b.value;
	// semantics for negative a or b are undefined, but in practice OK (gcc).
	// Nevertheless, be a little bit paranoid:
	if(r < 0)
		r = -r;
	if(a.value < 0)
		r = -r;
	return(makeCons(Numbers::internNative(q), makeCons(Numbers::internNative(r), NULL)));
}
static Integer integer00(0);
static Int int01(1);
static Int intM01(-1);
static NodeT divmod0Integer(const Numbers::Integer& a, Numbers::Integer b) {
	if(b == integer00)
		throw EvaluationException("division by zero");
	Numbers::Integer r(a);
	Numbers::Integer q;
	if((b.getSign() != Numbers::Integer::negative) ^ (a.getSign() != Numbers::Integer::negative)) {
		b = -b;
	}
	/* TODO just use bit shifts for positive powers of two, if that's faster. */
	r.divideWithRemainder(b, q);
	return(makeCons(toHeap(q), makeCons(toHeap(r), NULL)));
}
static NodeT divmod0Float(const Numbers::Float& a, const Numbers::Float& b) {
	NativeFloat avalue = a.value;
	NativeFloat bvalue = b.value;
	NativeFloat sign = ((bvalue >= 0) ^ (avalue >= 0)) ? (-1.0) : 1.0;
	if(avalue < 0.0)
		avalue = -avalue;
	if(bvalue < 0.0)
		bvalue = -bvalue;
	NativeFloat q = floor(avalue / bvalue) * sign; 
	// could use fmod, too. Not sure what for - we already have the result.
	NativeFloat r = a.value - q * b.value;
	// maybe convert to Integer (without loss, if possible)
	NativeInt qint = (NativeInt) q;
	if(qint == q)
		return(makeCons(Numbers::internNative(qint), makeCons(Numbers::internNative(r), NULL)));
	else
		return(makeCons(Numbers::internNative(q), makeCons(Numbers::internNative(r), NULL)));
	//return(makeOperation(Symbols::Sdivmod0, toHeap(a), toHeap(b)));
}

REGISTER_STR(Cons, {
	std::stringstream result;
	result << '[';
	result << str(node->head);
	for(Cons* vnode = Evaluators::evaluateToCons(node->tail); vnode; vnode = Evaluators::evaluateToCons(vnode->tail)) {
		result << ' ' << str(vnode->head);
	}
	result << ']';
	return(result.str());
})

static char hexdigits[17] = "0123456789abcdef";

static inline void printStrChar(unsigned int frontier, std::stringstream& sst, unsigned char c) {
		if(c == '"')
			sst << "\\\"";
		else if(c == '\\')
			sst << "\\\\";
		else if(c < 32 || c == 127 || c >= frontier) {
			switch(c) {
			case 7:
				sst << "\\a";
				break;
			case 8:
				sst << "\\b";
				break;
			case 9:
				sst << "\\t";
				break;
			case 10:
				sst << "\n";
				break;
			case 11:
				sst << "\\v";
				break;
			case 12:
				sst << "\\f";
				break;
			case 13:
				sst << "\\r";
				break;
			case 27:
				sst << "\\e";
				break;
			default:
				sst << "\\x" << hexdigits[c >> 4] << hexdigits[c & 0xF];
			}
		} else
			sst << c;
}
static inline std::string str1(const Str* node) {
	std::stringstream sst;
	const char* item;
	unsigned char c;
	size_t len = node->size;
	sst << "\"";
	for(item = (const char*) Evaluators::pointerFromNode((NodeT) node); (c = *item), len > 0; ++item, --len) {
		printStrChar(128, sst, c);
	}
	sst << "\"";
	return(sst.str());
}
static std::string strStr(const Str* node) {
	try {
		std::stringstream sst;
		const char* item;
		unsigned char c;
		int UTF8Count = (-1);
		size_t len = node->size;
		sst << "\"";
		for(item = (const char*) Evaluators::pointerFromNode((NodeT) node); (c = *item), len > 0; ++item, --len) {
			if(UTF8Count == (-1)) {
				if(c >= 0x80) {
					if(c == 0xC0 || c == 0xC1 || c >= 0xF5) { // funny extra limits by RFC 3629
						throw "invalid UTF-8";
					}
					UTF8Count = //(c >= 0xFC) ? 6 :
					            //(c >= 0xF8) ? 5 : 
					            (c >= 0xF0) ? 4 : 
					            (c >= 0xE0) ? 3 :
					            (c >= 0xC0) ? 2 :
					            (-1);
					if(UTF8Count == -1)
						throw "invalid UTF-8";
					--UTF8Count;
				} // else handle normally
			} else { // if(UTF8Count != -1) {
				if(c >= 0x80 && c < 0xC0) { // continued UTF-8 sequence
					--UTF8Count;
					if(UTF8Count < 0)
						throw "invalid UTF-8";
					else if(UTF8Count == 0)
						UTF8Count = (-1);
					// else wait for the next
				} else
					throw "invalid UTF-8";
			}
			printStrChar(255, sst, c);
		}
		sst << "\"";
		// TODO also detect some invalid or inconvenient codepoints? (invisible things etc)
		return(sst.str());
	} catch (const char* s) {
		// if the string is not valid UTF-8, revert to printing it in ASCII with escapes only.
		return(str1(node));
	}
}
REGISTER_STR(Box, return(str(node->fRepr));)

REGISTER_STR(Str, {
	return(strStr(node));
})
REGISTER_STR(Application,  {
	std::stringstream result;
	result << '(';
	result << str(node->operator_);
	result << ' ';
	result << str(node->operand);
	result << ')';
	return(result.str());
})
REGISTER_STR(Abstraction, {
	std::stringstream result;
	result << "(\\";
	result << str(node->parameter);
	result << ' ';
	result << str(node->body);
	result << ')';
	return(result.str());
})
REGISTER_STR(Keyword, return(std::string("@") + (node->name));)
REGISTER_STR(Symbol, return(node->name);)
REGISTER_STR(SymbolReference, return(Evaluators::str(node->symbol));)
static StrRegistration* root;
StrRegistration* registerStr(StrRegistration* n) {
	StrRegistration* oldRoot = root;
	root = n;
	return(oldRoot);
}
// TODO move this into the language runtime
std::string str(NodeT node) {
	if(node == NULL)
		return("[]");
	else if(root)
		return(root->call(node));
	else
		return("<node>");
}
static bool bDidWorldRun = false;
void resetWorld(void) {
	bDidWorldRun = false;
}
static inline NodeT bug(NodeT f) {
	abort();
	return(f);
}
#ifndef STRICT_BUILTINS
static NodeT fetchValueAndWorld(NodeT n) {
	Cons* cons = Evaluators::evaluateToCons(reduce(n));
	if(!cons)
		return(FALLBACK); /* WTF */
	// DO NOT REMOVE because it is possible that the monad only changes the world even though we don't care about the result.
	Evaluators::evaluateToCons(cons->tail);
	return(cons->head);
}
#define WORLD Numbers::internNative((Numbers::NativeInt) 42)
DEFINE_SIMPLE_LAZY_OPERATION(IORunner, fetchValueAndWorld(makeApplication(argument, WORLD)))
#else
DEFINE_SIMPLE_STRICT_OPERATION(IORunner, argument)
#endif

NodeT operator/(const Int& a, const Int& b) {
	if((a.value % b.value) == 0)
		return Numbers::internNative(a.value / b.value); // int
	else
	        return(Numbers::internNative((NativeFloat) a.value / (NativeFloat) b.value));
}
NodeT operator/(const Integer& a, const Integer& b) {
	if (b.isZero()) throw Evaluators::EvaluationException("Integer::operator /: division by zero");
	Integer q, r;
	r = a;
	r.divideWithRemainder(b, q);
	if(r.isZero()) {
		return new Integer(q);
	} else { // float...
		return toHeap(promoteToFloat(a) / promoteToFloat(b)); // FIXME faster?
	}
}

static NodeT makeACons(NodeT h, NodeT t, NodeT fallback) {
#ifndef STRICT_BUILTINS
	h = PREPARE(h);
	//t = reduce(t);
#endif
	return(makeCons(h, t));
}
static NodeT makeAPair(NodeT f, NodeT s, NodeT fallback) {
#ifndef STRICT_BUILTINS
	f = reduce(f);
	s = reduce(s);
#endif
	return(makePair(f, s));
}

/* make sure NOT to return a ratio here when it wasn't already one. The ratio constructor is divideARatio, not divideA. */
#define IMPLEMENT_NUMERIC_BUILTIN(N, op) \
NodeT N(NodeT a, NodeT b, NodeT fallback) { \
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a); \
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b); \
	if(aInt && bInt) { \
		return toHeap(*aInt op *bInt); \
	} else { \
		Numbers::Integer* aInteger = dynamic_cast<Numbers::Integer*>(a); \
		Numbers::Integer* bInteger = dynamic_cast<Numbers::Integer*>(b); \
		if(aInteger && bInteger) { \
			return toHeap((*aInteger) op (*bInteger)); \
		} else { \
			Numbers::Ratio* aRatio = dynamic_cast<Numbers::Ratio*>(a); \
			Numbers::Ratio* bRatio = dynamic_cast<Numbers::Ratio*>(b); \
			if(aRatio || bRatio) { \
				return(N##Ratio(a, b, fallback)); \
			} else if(aInteger && bInt) \
				return toHeap((*aInteger) op Integer(bInt->value)); \
			else if(aInt && bInteger) \
				return toHeap(Integer(aInt->value) op (*bInteger)); \
			Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a); \
			Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b); \
			if(aFloat && bFloat) \
				return toHeap(*aFloat op *bFloat); \
			else if(aFloat && bInt) \
				return toHeap(*aFloat op promoteToFloat(*bInt)); \
			else if(aInt && bFloat) \
				return toHeap(promoteToFloat(*aInt) op *bFloat); \
			else if(aFloat && bInteger) \
				return toHeap(*aFloat op promoteToFloat(*bInteger)); \
			else if(aInteger && bFloat) \
				return toHeap(promoteToFloat(*aInteger) op *bFloat); \
		} \
	} /* TODO optimize this: */ \
	/* TODO fish out neutral element */ \
	return(makeOperation(symbolFromStr(#op), a, b)); \
}

static NodeT divmod0ARatio(NodeT a, NodeT b, NodeT fallback);
NodeT divmod0A(NodeT a, NodeT b, NodeT fallback) {
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a);
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b);
	if(aInt && bInt) {
		return toHeap(divmod0Int(*aInt, *bInt));
	} else {
		Numbers::Integer* aInteger = dynamic_cast<Numbers::Integer*>(a);
		Numbers::Integer* bInteger = dynamic_cast<Numbers::Integer*>(b);
		if(aInteger && bInteger) {
			return toHeap(divmod0Integer((*aInteger), (*bInteger)));
		} else {
			Numbers::Ratio* aRatio = dynamic_cast<Numbers::Ratio*>(a); \
			Numbers::Ratio* bRatio = dynamic_cast<Numbers::Ratio*>(b); \
			if(aRatio || bRatio)
				return(divmod0ARatio(a, b, fallback));
			else if(aInteger && bInt)
				return toHeap(divmod0Integer((*aInteger), Integer(bInt->value)));
			else if(aInt && bInteger)
				return toHeap(divmod0Integer(Integer(aInt->value), (*bInteger)));
			Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a);
			Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b);
			if(aFloat && bFloat)
				return toHeap(divmod0Float(*aFloat, *bFloat));
			else if(aFloat && bInt) \
				return toHeap(divmod0Float(*aFloat, promoteToFloat(*bInt)));
			else if(aInt && bFloat) \
				return toHeap(divmod0Float(promoteToFloat(*aInt), *bFloat));
			else if(aFloat && bInteger) \
				return toHeap(divmod0Float(*aFloat, promoteToFloat(*bInteger)));
			else if(aInteger && bFloat) \
				return toHeap(divmod0Float(promoteToFloat(*aInteger), *bFloat));
		}
	}
	return(makeOperation(Symbols::Sdivmod0, a, b));
}
static NodeT compareAddrsLEA(NodeT a, NodeT b, NodeT fallback) {
	return(internNative((void*) a <= (void*) b));
}
static NodeT addrsEqualA(NodeT a, NodeT b, NodeT fallback) {
	return(internNative((void*) a == (void*) b));
}
NodeT leqA(NodeT a, NodeT b, NodeT fallback);
static inline bool equalP(NodeT a, NodeT b) {
	return(Evaluators::booleanFromNode(leqA(a, b, NULL)) && Evaluators::booleanFromNode(leqA(b, a, NULL)));
}
static NodeT gcd(NodeT a, NodeT b) {
	while(!equalP(b, &integer00)) {
		NodeT t = b;
		NodeT c = divmod0A(a, b, NULL);
		if(!c || !cons_P(c)) /* failed */
			throw EvaluationException("could not find greatest common divisor");
		b = get_cons_head(Evaluators::evaluateToCons(get_cons_tail(c))); // remainder
		a = t;
	}
	return a;
}
NodeT addA(NodeT a, NodeT b, NodeT fallback);
NodeT multiplyA(NodeT a, NodeT b, NodeT fallback);
NodeT subtractA(NodeT a, NodeT b, NodeT fallback);
NodeT divideA(NodeT a, NodeT b, NodeT fallback);
static NodeT simplifyRatio(NodeT r) {
	if(ratio_P(r)) {
		if(equalP(&int01, Ratio_getB(r)))
			return(Ratio_getA(r));
		else {
			NodeT a;
			NodeT b;
			NodeT g;
			a = Ratio_getA(r);
			b = Ratio_getB(r);
			if(!a || !b)
				return(r);
			try {
				g = gcd(a, b);
			} catch(EvaluationException e) {
				return(r);
			}
			if(Evaluators::booleanFromNode(leqA(g, &int01, NULL)) && Evaluators::booleanFromNode(leqA(&intM01, g, NULL))) {
			} else {
				a = divideA(a, g, NULL);
				b = divideA(b, g, NULL);
			}
			if(equalP(&int01, b))
				return(a);
			else
				return(makeRatio(a, b));
		}
	} else {
		return(r);
	}
}
static NodeT addARatio(NodeT a, NodeT b, NodeT fallback) {
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	if(equalP(Ratio_getB(a), Ratio_getB(b)))
		return(simplifyRatio(makeRatio(
			addA(Ratio_getA(a), Ratio_getA(b), NULL),
			Ratio_getB(a)
		)));
	else
		return(simplifyRatio(makeRatio(
			addA(multiplyA(Ratio_getA(a), Ratio_getB(b), NULL), multiplyA(Ratio_getB(a), Ratio_getA(b), NULL), NULL),
			multiplyA(Ratio_getB(a), Ratio_getB(b), NULL)
		)));
}
static NodeT subtractARatio(NodeT a, NodeT b, NodeT fallback) {
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	if(equalP(Ratio_getB(a), Ratio_getB(b)))
		return(simplifyRatio(makeRatio(
			subtractA(Ratio_getA(a), Ratio_getA(b), NULL),
			Ratio_getB(a)
		)));
	else
		return(simplifyRatio(makeRatio(
			subtractA(multiplyA(Ratio_getA(a), Ratio_getB(b), NULL), multiplyA(Ratio_getB(a), Ratio_getA(b), NULL), NULL),
			multiplyA(Ratio_getB(a), Ratio_getB(b), NULL)
		)));
}
static NodeT multiplyARatio(NodeT a, NodeT b, NodeT fallback) {
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	return(simplifyRatio(makeRatio(
		multiplyA(Ratio_getA(a), Ratio_getA(b), NULL), 
		multiplyA(Ratio_getB(a), Ratio_getB(b), NULL)
	)));
}
static NodeT divideARatio(NodeT a, NodeT b, NodeT fallback) {
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	return(simplifyRatio(makeRatio(
		multiplyA(Ratio_getA(a), Ratio_getB(b), NULL), 
		multiplyA(Ratio_getB(a), Ratio_getA(b), NULL)
	)));
}
static NodeT leqARatio(NodeT a, NodeT b, NodeT fallback) {
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	return(leqA(
		multiplyA(Ratio_getA(a), Ratio_getB(b), NULL), 
		multiplyA(Ratio_getB(a), Ratio_getA(b), NULL),
		NULL
	));
}
static NodeT divmod0ARatio(NodeT a, NodeT b, NodeT fallback) {
	NodeT q = divideARatio(a, b, NULL); // TODO fallback
	if(ratio_P(q)) {
		NodeT b = divmod0A(Ratio_getA(q), Ratio_getB(q), NULL); // TODO fallback
		if(b)
			q = get_cons_head(b);
	}
	// b*q + rem = a
	// d = a - b*q
	NodeT rem = subtractA(a, multiplyA(b, q, NULL), NULL); // TODO fallback
	return(makeCons(q, makeCons(rem, NULL)));
}
DEFINE_BINARY_OPERATION(Conser, makeACons)
DEFINE_BINARY_OPERATION(Pairer, makeAPair)
DEFINE_SIMPLE_STRICT_OPERATION(ConsP, cons_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(PairP, pair_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(NilP, nil_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(HeadGetter, ((cons_P(argument)) ? get_cons_head(argument) : str_P(argument) ? Numbers::internNative((Numbers::NativeInt) *((unsigned char*) Evaluators::pointerFromNode(argument))) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(TailGetter, ((cons_P(argument)) ? PREPARE(get_cons_tail(argument)) : str_P(argument) ? makeStrSlice((Str*) argument, 1) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(FstGetter, ((pair_P(argument)) ? Evaluators::get_pair_first(argument) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(SndGetter, ((pair_P(argument)) ? Evaluators::get_pair_second(argument) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(StrP, str_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(KeywordP, keyword_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(SymbolP, symbol_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(SymbolFromStrGetter, (str_P(argument) ? symbolFromStr(stringFromNode(argument)) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(KeywordFromStrGetter, (str_P(argument) ? keywordFromStr(stringFromNode(argument)) : FALLBACK))
DEFINE_SIMPLE_STRICT_OPERATION(KeywordStr, (keyword_P(argument) ? makeStr(get_keyword_name(argument)) : FALLBACK))
IMPLEMENT_NUMERIC_BUILTIN(addA, +)
DEFINE_BINARY_STRICT2_OPERATION(Adder, addA)
IMPLEMENT_NUMERIC_BUILTIN(subtractA, -)
DEFINE_BINARY_STRICT2_OPERATION(Subtractor, subtractA)
IMPLEMENT_NUMERIC_BUILTIN(multiplyA, *)
DEFINE_BINARY_STRICT2_OPERATION(Multiplicator, multiplyA)
IMPLEMENT_NUMERIC_BUILTIN(divideA, /)
DEFINE_BINARY_STRICT2_OPERATION(Divider, divideARatio)
DEFINE_BINARY_STRICT2_OPERATION(QModulator2, divmod0A)
/* TODO "non-numeric" comparison (i.e. strings) */
/* FIXME
These procedures [should] return #t if their arguments are (respectively): equal, monotonically increasing, monotonically decreasing, monotonically nondecreasing, or monotonically nonincreasing, and #f otherwise.

(= +inf.0 +inf.0)                   ⇒  #t
(= -inf.0 +inf.0)                   ⇒  #f
(= -inf.0 -inf.0)                   ⇒  #t

For any real number object x that is neither infinite nor NaN:

(< -inf.0 x +inf.0))                ⇒  #t
(> +inf.0 x -inf.0))                ⇒  #t

For any number object z:
(= +nan.0 z)                       ⇒  #f

For any real number object x:
(< +nan.0 x)                       ⇒  #f
(> +nan.0 x)                       ⇒  #f

These predicates must be transitive.
*/
IMPLEMENT_NUMERIC_BUILTIN(leqA, <=)
DEFINE_BINARY_OPERATION(LEComparer, leqA)
DEFINE_BINARY_STRICT2_OPERATION(AddrLEComparer, compareAddrsLEA)
DEFINE_BINARY_STRICT2_OPERATION(SymbolEqualityChecker, addrsEqualA)

static NodeT makeApplicationB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	NodeT operator_ = iter->second;
	++iter;
	NodeT operand = iter->second;
	//++iter;
	return(makeApplication(operator_, operand));
}
static NodeT makeAbstractionB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	NodeT parameter = iter->second;
	++iter;
	NodeT body = iter->second;
	//++iter;
	return(makeAbstraction(parameter, body));
}
/* INTERNAL! */
#define WRAP_PARSE_MATH(n, B, P) \
static NodeT n(Values::NodeT OPL, FILE* inputFile, CXXArguments& arguments, CXXArguments::const_iterator& iter, const CXXArguments::const_iterator& endIter) { \
	int position = 0; \
	NodeT name = NULL; \
	NodeT terminator = NULL; \
	for(++iter; iter != endIter; ++iter) { \
		if(iter->first) { \
			if(iter->first == keywordFromStr("position:")) { \
				position = Evaluators::intFromNode(iter->second); \
			} else if(iter->first == keywordFromStr("name:")) { \
				name = iter->second; \
			} else if(iter->first == keywordFromStr("terminator:")) { \
				terminator = iter->second; /* should be Symbol */ \
			} \
		} else \
			break; \
	} \
	try { \
		if(!terminator) \
			terminator = Symbols::SlessEOFgreater; \
		P parser; \
		parser.push(inputFile, position, name ? Evaluators::stringFromNode(name) : ""); \
		return(B); \
	} catch(...) { \
		return(NULL); /* FIXME */ \
	} \
}
WRAP_PARSE_MATH(parseMath, parser.parse(OPL, terminator), Scanners::MathParser);
WRAP_PARSE_MATH(parseParens, FNRESULT_FETCHINT(parser.parseMatchingParens(FNARG_FETCH(int))), Scanners::Scanner);

static NodeT makeFileMathParserB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	CXXArguments::const_iterator endIter = arguments.end();
	NodeT OPL = Evaluators::nodeFromNode(iter->second);
	++iter;
	NodeT inputFile = iter->second;
	FETCH_WORLD(iter);
	return(CHANGED_WORLD(parseMath(OPL, (FILE*) Evaluators::pointerFromNode(inputFile), arguments, iter, endIter)));
}
static NodeT parseStrMathB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	CXXArguments::const_iterator endIter = arguments.end();
	NodeT OPL = Evaluators::nodeFromNode(iter->second);
	++iter;
	const char* command = Evaluators::stringFromNode(iter->second);
	FILE* inputFile = fmemopen((void*) command, strlen(command), "r");
	// FIXME if !inputFile
	try {
		NodeT result = parseMath(OPL, inputFile, arguments, iter, endIter);
		fclose(inputFile);
		return(result);
	} catch(...) {
		fclose(inputFile);
		return(NULL); // FIXME
	}
}
static NodeT parseStrParensB(NodeT options, NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	CXXArguments::const_iterator endIter = arguments.end();
	const char* command = Evaluators::stringFromNode(iter->second);
	FILE* inputFile = fmemopen((void*) command, strlen(command), "r");
	// FIXME if !inputFile
	try {
		NodeT result = parseParens(NULL, inputFile, arguments, iter, endIter);
		fclose(inputFile);
		return(result);
	} catch(...) {
		fclose(inputFile);
		return(NULL); // FIXME
	}
}
static NodeT getFreeVariablesA(NodeT expr) {
	HashTable d;
	Evaluators::getFreeVariables(expr, d);
	return(listFromHashTable(d.begin(), d.end()));
}

#if 0
/* FIXME */
static NodeT makeBorg(NodeT foreigner) {
	NodeT OO = import_module(NULL, makeStr("OO"));
	NodeT List = import_module(NULL, makeStr("List"));
	NodeT Logic = import_module(NULL, makeStr("Logic"));
	// \name (\x  if (nil? x) ((requireModule "OO").Object name) x) (foreigner name)
	return(makeAbstraction(name, Evaluators::close(x, makeApplication(foreigner, name), 
	         makeApplication(Symbols::Sif, ))));
}
#endif
static inline NodeT ensureApplication(NodeT node) {
	if(!application_P(node))
		throw EvaluationException("argument is not an application");
	return(node);
}
static inline NodeT ensureAbstraction(NodeT node) {
	if(!abstraction_P(node))
		throw EvaluationException("argument is not an abstraction");
	return(node);
}
DEFINE_FULL_OPERATION(ApplicationMaker, return(makeApplicationB(fn, argument)))
DEFINE_SIMPLE_STRICT_OPERATION(ApplicationP, application_P(argument))
DEFINE_SIMPLE_STRICT_OPERATION(ApplicationOperatorGetter, get_application_operator(ensureApplication(argument)))
DEFINE_SIMPLE_STRICT_OPERATION(ApplicationOperandGetter, get_application_operand(ensureApplication(argument)))

DEFINE_FULL_OPERATION(AbstractionMaker, return(makeAbstractionB(fn, argument)))
DEFINE_SIMPLE_STRICT_OPERATION(AbstractionP, abstraction_P(PREPARE(argument)))
DEFINE_SIMPLE_STRICT_OPERATION(AbstractionParameterGetter, get_abstraction_parameter(ensureAbstraction(argument)))
DEFINE_SIMPLE_STRICT_OPERATION(AbstractionBodyGetter, get_abstraction_body(ensureAbstraction(argument)))

static Numbers::NativeFloat specialFloatValueOrZeroA(NodeT node) {
	Numbers::NativeFloat result;
	/* FIXME special-case fractions (after we found out what to do).
	   If the denominator is 0, it's certainly not a normal number.
	   It's probably (positive or negative) infinity then.
	   When is it NaN? 
	   What about 0/0 ? */
	if(Numbers::float_P(node) && Numbers::toNativeFloat(node, result))
		return(result);
	else
		return(0.0);
}
DEFINE_SIMPLE_STRICT_OPERATION(InfinityChecker, Evaluators::internNative(isinf(specialFloatValueOrZeroA(argument))))
DEFINE_SIMPLE_STRICT_OPERATION(NanChecker, Evaluators::internNative(isnan(specialFloatValueOrZeroA(argument)) != 0))

DEFINE_FULL_OPERATION2(FileMathParser, makeFileMathParserB)
DEFINE_FULL_OPERATION2(StrMathParser, parseStrMathB)
DEFINE_FULL_OPERATION2(StrParenParser, parseStrParensB)
DEFINE_SIMPLE_STRICT_OPERATION(FreeVariablesGetter, getFreeVariablesA(argument))
DEFINE_SIMPLE_STRICT_OPERATION(DynamicBuiltinGetter, getDynamicBuiltinA(argument))

REGISTER_BUILTIN(DynamicBuiltinGetter, 1, 0, symbolFromStr("dynamicBuiltin"))
REGISTER_BUILTIN(FreeVariablesGetter, 1, 0, symbolFromStr("freeVariables"))
REGISTER_BUILTIN(Conser, 2, 0, symbolFromStr(":"))
REGISTER_BUILTIN(Pairer, 2, 0, symbolFromStr(","))
REGISTER_BUILTIN(ConsP, 1, 0, symbolFromStr("cons?"))
REGISTER_BUILTIN(PairP, 1, 0, symbolFromStr("pair?"))
REGISTER_BUILTIN(NilP, 1, 0, symbolFromStr("nil?"))
REGISTER_BUILTIN(HeadGetter, 1, 0, symbolFromStr("head"))
REGISTER_BUILTIN(TailGetter, 1, 0, symbolFromStr("tail"))
REGISTER_BUILTIN(FstGetter, 1, 0, symbolFromStr("fst"))
REGISTER_BUILTIN(SndGetter, 1, 0, symbolFromStr("snd"))
REGISTER_BUILTIN(Adder, 2, 0, symbolFromStr("+"))
REGISTER_BUILTIN(Subtractor, 2, 0, symbolFromStr("-"))
REGISTER_BUILTIN(Multiplicator, 2, 0, symbolFromStr("*"))
REGISTER_BUILTIN(Divider, 2, 0, symbolFromStr("/"))
REGISTER_BUILTIN(QModulator2, 2, 0, symbolFromStr("divmod0"))
REGISTER_BUILTIN(LEComparer, 2, 0, symbolFromStr("<="))
REGISTER_BUILTIN(StrP, 1, 0, symbolFromStr("str?"))
REGISTER_BUILTIN(SymbolP, 1, 0, symbolFromStr("symbol?"))
REGISTER_BUILTIN(AddrLEComparer, 2, 0, symbolFromStr("addrsLE?"))
REGISTER_BUILTIN(SymbolEqualityChecker, 2, 0, symbolFromStr("symbolsEqual?"))
REGISTER_BUILTIN(KeywordP, 1, 0, symbolFromStr("keyword?"))
REGISTER_BUILTIN(SymbolFromStrGetter, 1, 0, symbolFromStr("symbolFromStr"))
REGISTER_BUILTIN(KeywordFromStrGetter, 1, 0, symbolFromStr("keywordFromStr"))
REGISTER_BUILTIN(KeywordStr, 1, 0, symbolFromStr("keywordStr"))
REGISTER_BUILTIN(IORunner, 1, 0, symbolFromStr("runIO"))
REGISTER_BUILTIN(ApplicationMaker, (-2), 0, symbolFromStr("makeApp"))
REGISTER_BUILTIN(ApplicationP, 1, 0, symbolFromStr("app?"))
REGISTER_BUILTIN(ApplicationOperatorGetter, 1, 0, symbolFromStr("appOperator"))
REGISTER_BUILTIN(ApplicationOperandGetter, 1, 0, symbolFromStr("appOperand"))
REGISTER_BUILTIN(AbstractionMaker, (-2), 0, symbolFromStr("makeFn"))
REGISTER_BUILTIN(AbstractionP, 1, 0, symbolFromStr("fn?"))
REGISTER_BUILTIN(AbstractionParameterGetter, 1, 0, symbolFromStr("fnParam"))
REGISTER_BUILTIN(AbstractionBodyGetter, 1, 0, symbolFromStr("fnBody"))
REGISTER_BUILTIN(FileMathParser, (-3), 0, symbolFromStr("parseMath!"))
REGISTER_BUILTIN(StrMathParser, (-2), 0, symbolFromStr("parseMathStr"))
REGISTER_BUILTIN(StrParenParser, (-2), 0, symbolFromStr("parseMathStr"))
/* Numeric Mathematics */
REGISTER_BUILTIN(InfinityChecker, 1, 0, symbolFromStr("infinite?"))
REGISTER_BUILTIN(NanChecker, 1, 0, symbolFromStr("nan?"))

#ifndef STRICT_BUILTINS
CXXArguments CXXfromArgumentsU(NodeT options, NodeT argument, int backwardsIndexOfArgumentNotToReduce) {
	CXXArguments result;
	NodeT v;
	NodeT p;
	int i = 1;
	bool B_pending_value = false;
	//assert(options); // can happen when calling manually, so commented out.
	p = backwardsIndexOfArgumentNotToReduce == 0 ? argument : reduce(argument);
	B_pending_value = true;
	NodeT self;
	for(self = options; curried_operation_P(self); self = get_curried_operation_operation(self), ++i) {
		NodeT arg = get_curried_operation_argument(self);
		v = i == backwardsIndexOfArgumentNotToReduce ? arg : reduce(arg); // backwards...
		if(B_pending_value && keyword_P(v)) {
			result.push_back(std::make_pair(v, p));
			p = NULL;
			B_pending_value = false;
		} else {
			if(B_pending_value)
				result.push_front(std::pair<NodeT, NodeT>(NULL, p));
			p = v;
			B_pending_value = true;
		}
	}
	if(B_pending_value) {
		B_pending_value = false;
		result.push_front(std::pair<NodeT, NodeT>(NULL, p));
	}
	return(result);
}
#else /* strict */
CXXArguments CXXfromArgumentsU(NodeT options, NodeT argument, int backwardsIndexOfArgumentNotToReduce) {
	CXXArguments result;
	NodeT v;
	NodeT p;
	int i = 1;
	bool B_pending_value = false;
	assert(options);
	p = argument; // backwardsIndexOfArgumentNotToReduce == 0 ? argument : reduce(argument);
	B_pending_value = true;
	NodeT self;
	for(self = options; curried_operation_P(self); self = get_curried_operation_operation(self), ++i) {
		NodeT arg = get_curried_operation_argument(self);
		//v = i == backwardsIndexOfArgumentNotToReduce ? arg : reduce(arg); // backwards...
		v = arg;
		if(B_pending_value && keyword_P(v)) {
			result.push_back(std::make_pair(v, p));
			p = NULL;
			B_pending_value = false;
		} else {
			if(B_pending_value)
				result.push_front(std::pair<NodeT, NodeT>(NULL, p));
			p = v;
			B_pending_value = true;
		}
	}
	if(B_pending_value) {
		B_pending_value = false;
		result.push_front(std::pair<NodeT, NodeT>(NULL, p));
	}
	return(result);
}
#endif
CXXArguments CXXfromArguments(NodeT options, NodeT argument) {
	return(CXXfromArgumentsU(options, argument, -1));
}
NodeT CXXgetKeywordArgumentValue(const CXXArguments& list, Values::NodeT key) {
	for(CXXArguments::const_iterator iter = list.begin(); iter != list.end(); ++iter)
		if(iter->first == key)
			return(iter->second);
	return(NULL);
}

}; /* end namespace Evaluators */
