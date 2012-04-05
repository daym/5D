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
#include "AST/AST"
#include "AST/Symbol"
#include "AST/Keyword"
#include "AST/HashTable"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Scanners/MathParser"
#include "Scanners/OperatorPrecedenceList"
#include "FFIs/FFIs"
#include "Numbers/Integer"
#include "Evaluators/Operation"
#include "FFIs/Allocators"
#include "Formatters/Math"
#include "Numbers/Real"
#include "Numbers/Ratio"

namespace Evaluators {

using namespace AST;
AST::NodeT churchTrue = Evaluators::annotate(AST::makeAbstraction(AST::symbolFromStr("t"), AST::makeAbstraction(AST::symbolFromStr("f"), AST::symbolFromStr("t"))));
//Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
AST::NodeT churchFalse = Evaluators::annotate(AST::makeAbstraction(AST::symbolFromStr("t"), AST::makeAbstraction(AST::symbolFromStr("f"), AST::symbolFromStr("f"))));
//AST::NodeT churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
AST::NodeT internNative(bool value) {
	return(value ? churchTrue : churchFalse);
}
AST::NodeT internNative(const char* value) {
	return(AST::makeStr(value));
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
static std::map<AST::Symbol*, AST::NodeT> cachedDynamicBuiltins;
static AST::NodeT get_dynamic_builtin(const char* name) {
	if((name[0] >= '0' && name[0] <= '9') || name[0] == '-') { /* hello, number */
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
	} else
		return(NULL);
}
AST::NodeT provide_dynamic_builtins_impl(AST::NodeT body, AST::HashTable::const_iterator end_iter, AST::HashTable::const_iterator iter) {
	if(iter == end_iter)
		return(body);
	else {
		const char* name = iter->first;
		AST::NodeT value = get_dynamic_builtin(name);
		++iter;
		return(value ? Evaluators::close(AST::symbolFromStr(name), value, provide_dynamic_builtins_impl(body, end_iter, iter)) : provide_dynamic_builtins_impl(body, end_iter, iter));
	}
}
AST::NodeT provide_dynamic_builtins(AST::NodeT body) {
	AST::HashTable freeNames;
	AST::HashTable::const_iterator end_iter = freeNames.end();
	get_free_variables(body, freeNames);
	end_iter = freeNames.end();
	return(provide_dynamic_builtins_impl(body, end_iter, freeNames.begin()));
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
static AST::NodeT toHeap(AST::NodeT v) {
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
static AST::NodeT toHeap(bool v) {
	return(internNative(v));
}
using namespace AST;

static AST::NodeT divremInt(const Numbers::Int& a, const Numbers::Int& b) {
	if(b.value == 0)
		throw EvaluationException("division by zero");
	NativeInt q = a.value / b.value;
	NativeInt r = a.value % b.value;
	// semantics for negative a or b are undefined, but in practise OK (gcc).
	// Nevertheless, be a little bit paranoid:
	if(r < 0)
		r = -r;
	if(a.value < 0)
		r = -r;
	return(AST::makeCons(Numbers::internNative(q), AST::makeCons(Numbers::internNative(r), NULL)));
}
static Integer integer00(0);
static Int int01(1);
static Int intM01(-1);
static AST::NodeT divremInteger(const Numbers::Integer& a, Numbers::Integer b) {
	if(b == integer00)
		throw EvaluationException("division by zero");
	Numbers::Integer r(a);
	Numbers::Integer q;
	if((b.getSign() != Numbers::Integer::negative) ^ (a.getSign() != Numbers::Integer::negative)) {
		b = -b;
	}
	/* TODO just use bit shifts for positive powers of two, if that's faster. */
	r.divideWithRemainder(b, q);
	return(AST::makeCons(toHeap(q), AST::makeCons(toHeap(r), NULL)));
}
static AST::NodeT divremFloat(const Numbers::Float& a, const Numbers::Float& b) {
	NativeFloat avalue = a.value;
	NativeFloat bvalue = b.value;
	NativeFloat sign = ((bvalue >= 0) ^ (avalue >= 0)) ? (-1.0) : 1.0;
	if(avalue < 0.0)
		avalue = -avalue;
	if(bvalue < 0.0)
		bvalue = -bvalue;
	NativeFloat q = floor(avalue / bvalue) * sign;
	NativeFloat r = a.value - q * b.value;
	return(AST::makeCons(Numbers::internNative(q), AST::makeCons(Numbers::internNative(r), NULL)));
	//return(makeOperation(Symbols::Sdivrem, toHeap(a), toHeap(b)));
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

static std::string strStr(AST::Str* node) {
	std::stringstream sst;
	const char* item;
	unsigned char c;
	size_t len = node->size;
	sst << "\"";
	for(item = (const char*) node->native; (c = *item), len > 0; ++item, --len) {
		if(c == '"')
			sst << "\\\"";
		else if(c == '\\')
			sst << "\\\\";
		else if(c < 32) {
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
	sst << "\"";
	return(sst.str());
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
REGISTER_STR(SymbolReference, return(node->symbol->name);)
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
static inline AST::NodeT bug(AST::NodeT f) {
	abort();
	return(f);
}
static AST::NodeT fetchValueAndWorld(AST::NodeT n) {
	AST::Cons* cons = Evaluators::evaluateToCons(reduce(n));
	if(!cons)
		return(FALLBACK); /* WTF */
	// DO NOT REMOVE because it is possible that the monad only changes the world even though we don't care about the result.
	Evaluators::evaluateToCons(cons->tail);
	return(cons->head);
}
#define WORLD Numbers::internNative((Numbers::NativeInt) 42)
DEFINE_SIMPLE_OPERATION(IORunner, fetchValueAndWorld(makeApplication(argument, WORLD)))
AST::NodeT operator/(const Int& a, const Int& b) {
	if((a.value % b.value) == 0)
		return Numbers::internNative(a.value / b.value); // int
	else
	        return(Numbers::internNative((NativeFloat) a.value / (NativeFloat) b.value));
}
AST::NodeT operator/(const Integer& a, const Integer& b) {
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

static AST::NodeT makeACons(AST::NodeT h, AST::NodeT t, AST::NodeT fallback) {
	h = reduce(h);
	//t = reduce(t);
	return(makeCons(h, t));
}
/* make sure NOT to return a ratio here when it wasn't already one. The ratio constructor is divideARatio, not divideA. */
#define IMPLEMENT_NUMERIC_BUILTIN(N, op) \
AST::NodeT N(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) { \
	a = reduce(a); \
	b = reduce(b); \
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
	return(makeOperation(AST::symbolFromStr(#op), a, b)); \
}

static AST::NodeT divremARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
AST::NodeT divremA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a);
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b);
	if(aInt && bInt) {
		return toHeap(divremInt(*aInt, *bInt));
	} else {
		Numbers::Integer* aInteger = dynamic_cast<Numbers::Integer*>(a);
		Numbers::Integer* bInteger = dynamic_cast<Numbers::Integer*>(b);
		if(aInteger && bInteger) {
			return toHeap(divremInteger((*aInteger), (*bInteger)));
		} else {
			Numbers::Ratio* aRatio = dynamic_cast<Numbers::Ratio*>(a); \
			Numbers::Ratio* bRatio = dynamic_cast<Numbers::Ratio*>(b); \
			if(aRatio || bRatio)
				return(divremARatio(a, b, fallback));
			else if(aInteger && bInt)
				return toHeap(divremInteger((*aInteger), Integer(bInt->value)));
			else if(aInt && bInteger)
				return toHeap(divremInteger(Integer(aInt->value), (*bInteger)));
			Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a);
			Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b);
			if(aFloat && bFloat)
				return toHeap(divremFloat(*aFloat, *bFloat));
			else if(aFloat && bInt) \
				return toHeap(divremFloat(*aFloat, promoteToFloat(*bInt)));
			else if(aInt && bFloat) \
				return toHeap(divremFloat(promoteToFloat(*aInt), *bFloat));
			else if(aFloat && bInteger) \
				return toHeap(divremFloat(*aFloat, promoteToFloat(*bInteger)));
			else if(aInteger && bFloat) \
				return toHeap(divremFloat(promoteToFloat(*aInteger), *bFloat));
		}
	}
	return(makeOperation(Symbols::Sdivrem, a, b));
}
static AST::NodeT compareAddrsLEA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	return(internNative((void*) a <= (void*) b));
}
static AST::NodeT addrsEqualA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	return(internNative((void*) a == (void*) b));
}
/*
template<>
class std::hash<AST::NodeT> {
};
*/
static AST::NodeT mapGetFst2(AST::NodeT fallback, AST::Cons* c) {
	if(c == NULL) {
		AST::NodeT tail = fallback ? reduce(AST::makeApplication(fallback, Symbols::Sexports)) : NULL;
		return(tail);
	} else
		return(AST::makeCons(reduce(evaluateToCons(reduce(c->head))->head), mapGetFst2(fallback, evaluateToCons(c->tail))));
}
static AST::NodeT dispatchModule(AST::NodeT options, AST::NodeT argument) {
	/* parameters: <exports> <fallback> <key> 
	               2         1          0*/
	CXXArguments arguments = Evaluators::CXXfromArgumentsU(options, argument, 1);
	CXXArguments::const_iterator iter = arguments.begin();
	/*std::stringstream buffer;
	std::string v;
	int position = 0;
	Formatters::Math::print_CXX(new Scanners::OperatorPrecedenceList(), buffer, position, iter->second, 0, false);
	v = buffer.str();
	printf("%s\n", v.c_str());*/
	AST::Box* mBox = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::NodeT fallback = iter->second;
	++iter;
	AST::NodeT key = iter->second;
	AST::HashTable* m;
	if(!mBox) {
		throw Evaluators::EvaluationException("invalid symbol table entry (*)");
		return(NULL); // FIXME
	}
	if(dynamic_cast<AST::HashTable*>((AST::NodeT) mBox->native) == NULL) {
		//cons_P((AST::NodeT) mBox->native)) {
		m = new (UseGC) AST::HashTable;
		for(AST::Cons* table = (AST::Cons*) mBox->native; table; table = Evaluators::evaluateToCons(table->tail)) {
			AST::Cons* entry = evaluateToCons(reduce(table->head));
			//std::string v = str(entry);
			//printf("%s\n", v.c_str());
			AST::NodeT x_key = reduce(entry->head);
			AST::Cons* snd = evaluateToCons(entry->tail);
			if(!snd)
				throw Evaluators::EvaluationException("invalid symbol table entry");
			AST::NodeT value = reduce(snd->head);
			const char* name = AST::get_symbol1_name(x_key);
			if(m->find(name) == m->end())
				(*m)[name] = value;
		}
		AST::Cons* table = (AST::Cons*) mBox->native;
		mBox->native = m;
		(*m)["exports"] = AST::makeCons(Symbols::Sexports, mapGetFst2(fallback, table));
	}
	m = (AST::HashTable*) mBox->native;
	const char* name = AST::get_symbol1_name(key); 
	if(name) {
		/*HashTable::const_iterator b = m->begin();
		HashTable::const_iterator e = m->end();
		for(; b != e; ++b) {
			printf("%s<\n", b->first);
		}
		printf("searching \"%s\"\n", s->name);*/
		AST::HashTable::const_iterator iter = m->find(name);
		if(iter != m->end())
			return(iter->second);
		else {
			if(fallback) { // this should always hold
				return(reduce(AST::makeApplication(fallback, key)));
			} else { // this is a leftover
				std::stringstream sst;
				sst << "library does not contain '" << name;
				std::string v = sst.str();
				throw Evaluators::EvaluationException(GCx_strdup(v.c_str()));
			}
		}
	} else
		throw Evaluators::EvaluationException("not a symbol");
	return(NULL);
}
AST::NodeT leqA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
static inline bool equalP(AST::NodeT a, AST::NodeT b) {
	return(Evaluators::get_boolean(leqA(a, b, NULL)) && Evaluators::get_boolean(leqA(b, a, NULL)));
}
static AST::NodeT gcd(AST::NodeT a, AST::NodeT b) {
	while(!equalP(b, &integer00)) {
		AST::NodeT t = b;
		AST::NodeT c = divremA(a, b, NULL);
		if(!c || !cons_P(c)) /* failed */
			throw EvaluationException("could not find greatest common divisor");
		b = AST::get_cons_head(Evaluators::evaluateToCons(AST::get_cons_tail(c))); // remainder
		a = t;
	}
	return a;
}
AST::NodeT addA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
AST::NodeT multiplyA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
AST::NodeT subtractA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
AST::NodeT divideA(AST::NodeT a, AST::NodeT b, AST::NodeT fallback);
static AST::NodeT simplifyRatio(AST::NodeT r) {
	if(ratio_P(r)) {
		if(equalP(&int01, Ratio_getB(r)))
			return(Ratio_getA(r));
		else {
			AST::NodeT a;
			AST::NodeT b;
			AST::NodeT g;
			a = Ratio_getA(r);
			b = Ratio_getB(r);
			if(!a || !b)
				return(r);
			try {
				g = gcd(a, b);
			} catch(EvaluationException e) {
				return(r);
			}
			if(Evaluators::get_boolean(leqA(g, &int01, NULL)) && Evaluators::get_boolean(leqA(&intM01, g, NULL))) {
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
static AST::NodeT addARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
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
static AST::NodeT subtractARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
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
static AST::NodeT multiplyARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	return(simplifyRatio(makeRatio(
		multiplyA(Ratio_getA(a), Ratio_getA(b), NULL), 
		multiplyA(Ratio_getB(a), Ratio_getB(b), NULL)
	)));
}
static AST::NodeT divideARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	a = reduce(a);
	b = reduce(b);
	if(!ratio_P(a))
		a = makeRatio(a, &int01);
	if(!ratio_P(b))
		b = makeRatio(b, &int01);
	return(simplifyRatio(makeRatio(
		multiplyA(Ratio_getA(a), Ratio_getB(b), NULL), 
		multiplyA(Ratio_getB(a), Ratio_getA(b), NULL)
	)));
}
static AST::NodeT leqARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
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
static AST::NodeT divremARatio(AST::NodeT a, AST::NodeT b, AST::NodeT fallback) {
	AST::NodeT q = divideARatio(a, b, NULL); // TODO fallback
	if(ratio_P(q)) {
		AST::NodeT b = divremA(Ratio_getA(q), Ratio_getB(q), NULL); // TODO fallback
		if(b)
			q = get_cons_head(b);
	}
	// b*q + rem = a
	// d = a - b*q
	AST::NodeT rem = subtractA(a, multiplyA(b, q, NULL), NULL); // TODO fallback
	return(AST::makeCons(q, AST::makeCons(rem, NULL)));
}
DEFINE_BINARY_OPERATION(Conser, makeACons)
DEFINE_SIMPLE_OPERATION(ConsP, cons_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(NilP, nil_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(HeadGetter, ((argument = reduce(argument), cons_P(argument)) ? (((AST::Cons*) argument)->head) : str_P(argument) ? Numbers::internNative((Numbers::NativeInt) *((unsigned char*) ((AST::Str*) argument)->native)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(TailGetter, ((argument = reduce(argument), cons_P(argument)) ? reduce(((AST::Cons*) argument)->tail) : str_P(argument) ? AST::makeStrSlice((AST::Str*) argument, 1) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrP, str_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(KeywordP, keyword_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(SymbolP, symbol_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(SymbolFromStrGetter, (argument = reduce(argument), str_P(argument) ? AST::symbolFromStr(get_string(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(KeywordFromStrGetter, (argument = reduce(argument), str_P(argument) ? AST::keywordFromStr(get_string(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(KeywordStr, (argument = reduce(argument), keyword_P(argument) ? Evaluators::internNative(dynamic_cast<Keyword*>(argument)->name) : FALLBACK))
IMPLEMENT_NUMERIC_BUILTIN(addA, +)
DEFINE_BINARY_OPERATION(Adder, addA)
IMPLEMENT_NUMERIC_BUILTIN(subtractA, -)
DEFINE_BINARY_OPERATION(Subtractor, subtractA)
IMPLEMENT_NUMERIC_BUILTIN(multiplyA, *)
DEFINE_BINARY_OPERATION(Multiplicator, multiplyA)
IMPLEMENT_NUMERIC_BUILTIN(divideA, /)
DEFINE_BINARY_OPERATION(Divider, divideARatio)
DEFINE_BINARY_OPERATION(QModulator2, divremA)
/* TODO "non-numeric" comparison (i.e. strings) */
IMPLEMENT_NUMERIC_BUILTIN(leqA, <=)
DEFINE_BINARY_OPERATION(LEComparer, leqA)
DEFINE_BINARY_OPERATION(AddrLEComparer, compareAddrsLEA)
DEFINE_BINARY_OPERATION(SymbolEqualityChecker, addrsEqualA)
DEFINE_FULL_OPERATION(ModuleDispatcher, return(dispatchModule(fn, argument));)
static inline AST::NodeT makeModuleBox(AST::NodeT argument) {
	return AST::makeBox(argument, AST::makeApplication(&ModuleBoxMaker, argument));
}
DEFINE_SIMPLE_OPERATION(ModuleBoxMaker, makeModuleBox(reduce(argument)))

static AST::NodeT makeApplicationB(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::NodeT operator_ = iter->second;
	++iter;
	AST::NodeT operand = iter->second;
	//++iter;
	return(AST::makeApplication(operator_, operand));
}
static AST::NodeT makeAbstractionB(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	AST::NodeT parameter = iter->second;
	++iter;
	AST::NodeT body = iter->second;
	//++iter;
	return(AST::makeAbstraction(parameter, body));
}
/* INTERNAL! */
static AST::NodeT parseMath(Scanners::OperatorPrecedenceList* OPL, FILE* inputFile, CXXArguments& arguments, CXXArguments::const_iterator& iter, const CXXArguments::const_iterator& endIter) {
	int position = 0; // FIXME size_t
	AST::NodeT name = NULL;
	AST::Symbol* terminator = NULL;
	for(++iter; iter != endIter; ++iter) {
		if(iter->first) { // likely
			if(iter->first == AST::keywordFromStr("position:")) {
				position = Evaluators::get_int(iter->second);
			} else if(iter->first == AST::keywordFromStr("name:")) {
				name = iter->second;
			} else if(iter->first == AST::keywordFromStr("terminator:")) {
				terminator = dynamic_cast<AST::Symbol*>(iter->second);
			}
		}
	}
	try {
		if(!terminator)
			terminator = Symbols::SlessEOFgreater;
		Scanners::MathParser parser;
		parser.push(inputFile, position, name ? Evaluators::get_string(name) : "");
		return(parser.parse(OPL, terminator));
	} catch(...) {
		return(NULL); // FIXME
	}
}
static AST::NodeT makeFileMathParserB(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	CXXArguments::const_iterator endIter = arguments.end();
	Scanners::OperatorPrecedenceList* OPL = (Scanners::OperatorPrecedenceList*)(Evaluators::get_pointer(iter->second));
	++iter;
	AST::NodeT inputFile = iter->second;
	++iter;
	AST::NodeT world = iter->second;
	return(Evaluators::makeIOMonad(parseMath(OPL, (FILE*) Evaluators::get_pointer(inputFile), arguments, iter, endIter), world));
}
static AST::NodeT makeStrMathParserB(AST::NodeT options, AST::NodeT argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	CXXArguments::const_iterator endIter = arguments.end();
	Scanners::OperatorPrecedenceList* OPL = (Scanners::OperatorPrecedenceList*)(Evaluators::get_pointer(iter->second));
	++iter;
	const char* command = Evaluators::get_string(iter->second);
	FILE* inputFile = fmemopen((void*) command, strlen(command), "r");
	// FIXME if !inputFile
	try {
		AST::NodeT result = parseMath(OPL, inputFile, arguments, iter, endIter);
		fclose(inputFile);
		return(result);
	} catch(...) {
		fclose(inputFile);
		return(NULL); // FIXME
	}
}
#if 0
/* FIXME */
static AST::NodeT makeBorg(AST::NodeT foreigner) {
	AST::NodeT OO = import_module(NULL, AST::makeStr("OO"));
	AST::NodeT List = import_module(NULL, AST::makeStr("List"));
	AST::NodeT Logic = import_module(NULL, AST::makeStr("Logic"));
	// \name (\x  if (nil? x) ((requireModule "OO").Object name) x) (foreigner name)
	return(AST::makeAbstraction(name, Evaluators::close(x, AST::makeApplication(foreigner, name), 
	         AST::makeApplication(Symbols::Sif, ))));
}
#endif
static inline AST::NodeT ensureApplication(AST::NodeT node) {
	if(!application_P(node))
		throw EvaluationException("argument is not an application");
	return(node);
}
static inline AST::NodeT ensureAbstraction(AST::NodeT node) {
	if(!abstraction_P(node))
		throw EvaluationException("argument is not an abstraction");
	return(node);
}
DEFINE_FULL_OPERATION(ApplicationMaker, return(makeApplicationB(fn, argument)))
DEFINE_SIMPLE_OPERATION(ApplicationP, AST::application_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(ApplicationOperatorGetter, AST::get_application_operator(ensureApplication(reduce(argument))))
DEFINE_SIMPLE_OPERATION(ApplicationOperandGetter, AST::get_application_operand(ensureApplication(reduce(argument))))

DEFINE_FULL_OPERATION(AbstractionMaker, return(makeAbstractionB(fn, argument)))
DEFINE_SIMPLE_OPERATION(AbstractionP, AST::abstraction_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(AbstractionParameterGetter, AST::get_abstraction_parameter(ensureAbstraction(reduce(argument))))
DEFINE_SIMPLE_OPERATION(AbstractionBodyGetter, AST::get_abstraction_body(ensureAbstraction(reduce(argument))))

DEFINE_FULL_OPERATION(RFileMathParser, return(makeFileMathParserB(fn, argument)))
DEFINE_FULL_OPERATION(RStrMathParser, return(makeStrMathParserB(fn, argument)))

REGISTER_BUILTIN(Conser, 2, 0, AST::symbolFromStr(":"))
REGISTER_BUILTIN(ConsP, 1, 0, AST::symbolFromStr("cons?"))
REGISTER_BUILTIN(NilP, 1, 0, AST::symbolFromStr("nil?"))
REGISTER_BUILTIN(HeadGetter, 1, 0, AST::symbolFromStr("head"))
REGISTER_BUILTIN(TailGetter, 1, 0, AST::symbolFromStr("tail"))
REGISTER_BUILTIN(Adder, 2, 0, AST::symbolFromStr("+"))
REGISTER_BUILTIN(Subtractor, 2, 0, AST::symbolFromStr("-"))
REGISTER_BUILTIN(Multiplicator, 2, 0, AST::symbolFromStr("*"))
REGISTER_BUILTIN(Divider, 2, 0, AST::symbolFromStr("/"))
REGISTER_BUILTIN(QModulator2, 2, 0, AST::symbolFromStr("divrem"))
REGISTER_BUILTIN(LEComparer, 2, 0, AST::symbolFromStr("<="))
REGISTER_BUILTIN(StrP, 1, 0, AST::symbolFromStr("str?"))
REGISTER_BUILTIN(SymbolP, 1, 0, AST::symbolFromStr("symbol?"))
REGISTER_BUILTIN(AddrLEComparer, 2, 0, AST::symbolFromStr("addrsLE?"))
REGISTER_BUILTIN(SymbolEqualityChecker, 2, 0, AST::symbolFromStr("symbolsEqual?"))
REGISTER_BUILTIN(KeywordP, 1, 0, AST::symbolFromStr("keyword?"))
REGISTER_BUILTIN(SymbolFromStrGetter, 1, 0, AST::symbolFromStr("symbolFromStr"))
REGISTER_BUILTIN(KeywordFromStrGetter, 1, 0, AST::symbolFromStr("keywordFromStr"))
REGISTER_BUILTIN(KeywordStr, 1, 0, AST::symbolFromStr("keywordStr"))
REGISTER_BUILTIN(IORunner, 1, 0, AST::symbolFromStr("runIO"))
REGISTER_BUILTIN(ModuleDispatcher, (-3), 1, AST::symbolFromStr("dispatchModule"))
REGISTER_BUILTIN(ModuleBoxMaker, 1, 0, AST::symbolFromStr("makeModuleBox"))
REGISTER_BUILTIN(ApplicationMaker, (-2), 0, AST::symbolFromStr("makeApp"))
REGISTER_BUILTIN(ApplicationP, 1, 0, AST::symbolFromStr("app?"))
REGISTER_BUILTIN(ApplicationOperatorGetter, 1, 0, AST::symbolFromStr("appOperator"))
REGISTER_BUILTIN(ApplicationOperandGetter, 1, 0, AST::symbolFromStr("appOperand"))
REGISTER_BUILTIN(AbstractionMaker, (-2), 0, AST::symbolFromStr("makeFn"))
REGISTER_BUILTIN(AbstractionP, 1, 0, AST::symbolFromStr("fn?"))
REGISTER_BUILTIN(AbstractionParameterGetter, 1, 0, AST::symbolFromStr("fnParam"))
REGISTER_BUILTIN(AbstractionBodyGetter, 1, 0, AST::symbolFromStr("fnBody"))
REGISTER_BUILTIN(RFileMathParser, (-3), 0, AST::symbolFromStr("parseMath!"))
REGISTER_BUILTIN(RStrMathParser, (-2), 0, AST::symbolFromStr("parseMathStr"))

// FIXME make this GCable.
CXXArguments CXXfromArgumentsU(AST::NodeT options, AST::NodeT argument, int backwardsIndexOfArgumentNotToReduce) {
	CXXArguments result;
	AST::NodeT v;
	AST::NodeT p;
	int i = 1;
	bool B_pending_value = false;
	assert(options);
	p = backwardsIndexOfArgumentNotToReduce == 0 ? argument : reduce(argument);
	B_pending_value = true;
	Evaluators::CurriedOperation* self;
	for(self = dynamic_cast<Evaluators::CurriedOperation*>(options); self; self = dynamic_cast<Evaluators::CurriedOperation*>(self->fOperation), ++i) {
		v = i == backwardsIndexOfArgumentNotToReduce ? self->fArgument : reduce(self->fArgument); // backwards...
		if(B_pending_value && keyword_P(v)) {
			result.push_back(std::make_pair(dynamic_cast<AST::Keyword*>(v), p));
			p = NULL;
			B_pending_value = false;
		} else {
			if(B_pending_value)
				result.push_front(std::pair<AST::Keyword*, AST::NodeT>(NULL, p));
			p = v;
			B_pending_value = true;
		}
	}
	if(B_pending_value) {
		B_pending_value = false;
		result.push_front(std::pair<AST::Keyword*, AST::NodeT>(NULL, p));
	}
	return(result);
}
CXXArguments CXXfromArguments(AST::NodeT options, AST::NodeT argument) {
	return(CXXfromArgumentsU(options, argument, -1));
}
AST::NodeT CXXgetKeywordArgumentValue(const CXXArguments& list, AST::Keyword* key) {
	for(std::list<std::pair<AST::Keyword*, AST::NodeT> >::const_iterator iter = list.begin(); iter != list.end(); ++iter)
		if(iter->first == key)
			return(iter->second);
	return(NULL);
}

}; /* end namespace Evaluators */
