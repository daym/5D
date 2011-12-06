#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <limits.h>
#include "AST/AST"
#include "AST/Symbol"
#include "AST/Keyword"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Scanners/MathParser"
#include "Scanners/OperatorPrecedenceList"
#include "FFIs/FFIs"
#include "Numbers/Integer"
#include "Evaluators/Operation"

namespace Evaluators {

using namespace AST;
AST::Node* churchTrue = Evaluators::annotate(AST::makeAbstraction(AST::symbolFromStr("t"), AST::makeAbstraction(AST::symbolFromStr("f"), AST::symbolFromStr("t"))));
//Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
AST::Node* churchFalse = Evaluators::annotate(AST::makeAbstraction(AST::symbolFromStr("t"), AST::makeAbstraction(AST::symbolFromStr("f"), AST::symbolFromStr("f"))));
//AST::Node* churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
AST::Node* internNative(bool value) {
	return(value ? churchTrue : churchFalse);
}
AST::Node* internNative(const char* value) {
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
static std::map<AST::Symbol*, AST::Node*> cachedDynamicBuiltins;
static AST::Node* get_dynamic_builtin(AST::Symbol* symbol) {
	const char* name;
	name = symbol->name;
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
						return(symbol);
					if(c != '0')
						B_zero = false;
					v = v * unsigneds[10] + unsigneds[c - '0'];
				}
				return(new Integer(v, B_zero ? Integer::zero : B_negative ? Integer::negative : Integer::positive));
				//return(symbol); // TODO allow to hook into this.
			}
		}
		return(Numbers::internNative((NativeInt) value));
	} else
		return(NULL);
}
AST::Node* provide_dynamic_builtins_impl(AST::Node* body, std::set<AST::Symbol*>::const_iterator end_iter, std::set<AST::Symbol*>::const_iterator iter) {
	if(iter == end_iter)
		return(body);
	else {
		AST::Symbol* name = *iter;
		AST::Node* value = get_dynamic_builtin(name);
		++iter;
		return(value ? Evaluators::close(name, value, provide_dynamic_builtins_impl(body, end_iter, iter)) : provide_dynamic_builtins_impl(body, end_iter, iter));
	}
}
AST::Node* provide_dynamic_builtins(AST::Node* body) {
	std::set<AST::Symbol*> freeNames;
	std::set<AST::Symbol*>::const_iterator end_iter;
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
static AST::Node* toHeap(AST::Node* v) {
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
static AST::Node* toHeap(bool v) {
	return(internNative(v));
}
static AST::Node* addrEqualsP(AST::Node* a, AST::Node* b) {
	return(internNative(a == b));
}
static AST::Node* addrLEP(AST::Node* a, AST::Node* b) {
	return(internNative(a <= b));
}
using namespace AST;

static AST::Node* divmodInt(const Numbers::Int& a, const Numbers::Int& b) {
	if(b.value == 0)
		throw EvaluationException("division by zero");
	NativeInt q = a.value / b.value;
	NativeInt r = a.value % b.value; // FIXME semantics for negative numbers.
	return(AST::makeCons(Numbers::internNative(q), AST::makeCons(Numbers::internNative(r), NULL)));
}
static Integer integer00(0);
static AST::Node* divmodInteger(const Numbers::Integer& a, const Numbers::Integer& b) {
	if(b == integer00)
		throw EvaluationException("division by zero");
	Numbers::Integer r(a);
	Numbers::Integer q;
	r.divideWithRemainder(b, q);
	return(AST::makeCons(toHeap(q), AST::makeCons(toHeap(r), NULL)));
}
static AST::Node* divmodFloat(const Numbers::Float& a, const Numbers::Float& b) {
	if(b.value == 0.0)
		throw EvaluationException("division by zero");
	// FIXME
	//return(divmodInt((NativeInt) a.value, (NativeInt) b.value));
	return(makeOperation(Symbols::Sdivmod, toHeap(a), toHeap(b)));
}
static AST::Node* divmod(AST::Node* a, AST::Node* b) {
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a); \
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b); \
	if(aInt && bInt) { \
		return toHeap(divmodInt(*aInt, *bInt)); \
	} else { \
		Numbers::Integer* aInteger = dynamic_cast<Numbers::Integer*>(a);
		Numbers::Integer* bInteger = dynamic_cast<Numbers::Integer*>(b);
		if(aInteger && bInteger) {
			return toHeap(divmodInteger((*aInteger), (*bInteger)));
		} else {
			if(aInteger && bInt)
				return toHeap(divmodInteger((*aInteger), Integer(bInt->value)));
			else if(aInt && bInteger)
				return toHeap(divmodInteger(Integer(aInt->value), (*bInteger)));
			Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a);
			Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b);
			if(aFloat && bFloat)
				return toHeap(divmodFloat(*aFloat, *bFloat));
			else if(aFloat && bInt) \
				return toHeap(divmodFloat(*aFloat, promoteToFloat(*bInt)));
			else if(aInt && bFloat) \
				return toHeap(divmodFloat(promoteToFloat(*aInt), *bFloat));
			else if(aFloat && bInteger) \
				return toHeap(divmodFloat(*aFloat, promoteToFloat(*bInteger)));
			else if(aInteger && bFloat) \
				return toHeap(divmodFloat(promoteToFloat(*aInteger), *bFloat));
		}
	}
	return(makeOperation(Symbols::Sdivmod, a, b));
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

REGISTER_STR(Str, {
	std::stringstream sst;
	const char* item;
	char c;
	sst << "\"";
	for(item = node->text.c_str(); (c = *item); ++item) {
		if(c == '"')
			sst << '\\';
		else if(c == '\\')
			sst << '\\';
		/* TODO escape other things? not that useful... */
		sst << c;
	}
	sst << "\"";
	return(sst.str());
})
REGISTER_STR(Box, return("box");)
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
std::string str(Node* node) {
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
static inline AST::Node* bug(AST::Node* f) {
	abort();
	return(f);
}
static AST::Node* fetchValueAndWorld(AST::Node* n) {
	AST::Cons* cons = dynamic_cast<AST::Cons*>(n);
	if(!cons)
		return(FALLBACK); /* WTF */
	// DO NOT REMOVE:
	Evaluators::evaluateToCons(cons->tail);
	return(cons->head);
}
/* TODO reduce once: */
DEFINE_SIMPLE_OPERATION(WorldRunner, fetchValueAndWorld(reduce(makeApplication(reduce(argument), Numbers::internNative((Numbers::NativeInt) 42)))))

AST::Node* operator/(const Integer& a, const Integer& b) {
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
static AST::Node* makeACons(AST::Node* h, AST::Node* t, AST::Node* fallback) {
	h = reduce(h);
	//t = reduce(t);
	return(makeCons(h, t));
}
#define IMPLEMENT_NUMERIC_BUILTIN(N, op) \
AST::Node* N(AST::Node* a, AST::Node* b, AST::Node* fallback) { \
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
			if(aInteger && bInt) \
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
	return(makeOperation(AST::symbolFromStr(#op), a, b)); \
}

static AST::Node* divmodA(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a);
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b);
	if(aInt && bInt) {
		return toHeap(divmodInt(*aInt, *bInt));
	} else {
		Numbers::Integer* aInteger = dynamic_cast<Numbers::Integer*>(a);
		Numbers::Integer* bInteger = dynamic_cast<Numbers::Integer*>(b);
		if(aInteger && bInteger) {
			return toHeap(divmodInteger((*aInteger), (*bInteger)));
		} else {
			if(aInteger && bInt)
				return toHeap(divmodInteger((*aInteger), Integer(bInt->value)));
			else if(aInt && bInteger)
				return toHeap(divmodInteger(Integer(aInt->value), (*bInteger)));
			Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a);
			Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b);
			if(aFloat && bFloat)
				return toHeap(divmodFloat(*aFloat, *bFloat));
			else if(aFloat && bInt) \
				return toHeap(divmodFloat(*aFloat, promoteToFloat(*bInt)));
			else if(aInt && bFloat) \
				return toHeap(divmodFloat(promoteToFloat(*aInt), *bFloat));
			else if(aFloat && bInteger) \
				return toHeap(divmodFloat(*aFloat, promoteToFloat(*bInteger)));
			else if(aInteger && bFloat) \
				return toHeap(divmodFloat(promoteToFloat(*aInteger), *bFloat));
		}
	}
	return(makeOperation(Symbols::Sdivmod, a, b));
}
static AST::Node* compareAddrsLEA(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	return(internNative((void*) a <= (void*) b));
}
static AST::Node* addrsEqualA(AST::Node* a, AST::Node* b, AST::Node* fallback) {
	a = reduce(a);
	b = reduce(b);
	return(internNative((void*) a == (void*) b));
}
DEFINE_BINARY_OPERATION(Conser, makeACons)
DEFINE_SIMPLE_OPERATION(ConsP, cons_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(NilP, nil_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(HeadGetter, ((argument = reduce(argument), cons_P(argument)) ? (((AST::Cons*) argument)->head) : FALLBACK))
DEFINE_SIMPLE_OPERATION(TailGetter, ((argument = reduce(argument), cons_P(argument)) ? reduce(((AST::Cons*) argument)->tail) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrP, str_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(KeywordP, keyword_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(SymbolP, symbol_P(reduce(argument)))
DEFINE_SIMPLE_OPERATION(SymbolFromStrGetter, (argument = reduce(argument), str_P(argument) ? AST::symbolFromStr(get_native_string(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(KeywordFromStrGetter, (argument = reduce(argument), str_P(argument) ? AST::keywordFromStr(get_native_string(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(ListFromStrGetter, (argument = reduce(argument), str_P(argument) ? Evaluators::listFromStr(dynamic_cast<Str*>(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(StrFromListGetter, (argument = reduce(argument), (cons_P(argument) || nil_P(argument)) ? Evaluators::strFromList(dynamic_cast<Cons*>(argument)) : FALLBACK))
DEFINE_SIMPLE_OPERATION(KeywordStr, (argument = reduce(argument), keyword_P(argument) ? Evaluators::internNative(dynamic_cast<Keyword*>(argument)->name) : FALLBACK))
IMPLEMENT_NUMERIC_BUILTIN(addA, +)
DEFINE_BINARY_OPERATION(Adder, addA)
IMPLEMENT_NUMERIC_BUILTIN(subtractA, -)
DEFINE_BINARY_OPERATION(Subtractor, subtractA)
IMPLEMENT_NUMERIC_BUILTIN(multiplyA, *)
DEFINE_BINARY_OPERATION(Multiplicator, multiplyA)
IMPLEMENT_NUMERIC_BUILTIN(divideA, /)
DEFINE_BINARY_OPERATION(Divider, divideA)
DEFINE_BINARY_OPERATION(QModulator, divmodA)
IMPLEMENT_NUMERIC_BUILTIN(leqA, <=)
DEFINE_BINARY_OPERATION(LEComparer, leqA)
DEFINE_BINARY_OPERATION(AddrLEComparer, compareAddrsLEA)
DEFINE_BINARY_OPERATION(SymbolEqualityChecker, addrsEqualA)

REGISTER_BUILTIN(Conser, 2, 0, AST::symbolFromStr(":"))
REGISTER_BUILTIN(ConsP, 1, 0, AST::symbolFromStr("cons?"))
REGISTER_BUILTIN(NilP, 1, 0, AST::symbolFromStr("nil?"))
REGISTER_BUILTIN(HeadGetter, 1, 0, AST::symbolFromStr("head"))
REGISTER_BUILTIN(TailGetter, 1, 0, AST::symbolFromStr("tail"))
REGISTER_BUILTIN(Adder, 2, 0, AST::symbolFromStr("+"))
REGISTER_BUILTIN(Subtractor, 2, 0, AST::symbolFromStr("-"))
REGISTER_BUILTIN(Multiplicator, 2, 0, AST::symbolFromStr("*"))
REGISTER_BUILTIN(Divider, 2, 0, AST::symbolFromStr("/"))
REGISTER_BUILTIN(QModulator, 2, 0, AST::symbolFromStr("divmod"))
REGISTER_BUILTIN(LEComparer, 2, 0, AST::symbolFromStr("<="))
REGISTER_BUILTIN(StrP, 1, 0, AST::symbolFromStr("str?"))
REGISTER_BUILTIN(SymbolP, 1, 0, AST::symbolFromStr("symbol?"))
REGISTER_BUILTIN(AddrLEComparer, 2, 0, AST::symbolFromStr("addrsLE?"))
REGISTER_BUILTIN(SymbolEqualityChecker, 2, 0, AST::symbolFromStr("symbolsEqual?"))
REGISTER_BUILTIN(KeywordP, 1, 0, AST::symbolFromStr("keyword?"))
REGISTER_BUILTIN(SymbolFromStrGetter, 1, 0, AST::symbolFromStr("symbolFromStr"))
REGISTER_BUILTIN(KeywordFromStrGetter, 1, 0, AST::symbolFromStr("keywordFromStr"))
REGISTER_BUILTIN(KeywordStr, 1, 0, AST::symbolFromStr("keywordStr"))
REGISTER_BUILTIN(ListFromStrGetter, 1, 0, AST::symbolFromStr("listFromStr"))
REGISTER_BUILTIN(StrFromListGetter, 1, 0, AST::symbolFromStr("strFromList"))
REGISTER_BUILTIN(WorldRunner, 1, 0, AST::symbolFromStr("runWorld"))

std::list<std::pair<AST::Keyword*, AST::Node*> > CXXfromArguments(AST::Node* options, AST::Node* argument) {
	std::list<std::pair<AST::Keyword*, AST::Node*> > result;
	AST::Node* v;
	AST::Node* p;
	bool B_pending_value = false;
	assert(options);
	p = reduce(argument);
	B_pending_value = true;
	Evaluators::CurriedOperation* self;
	for(self = dynamic_cast<Evaluators::CurriedOperation*>(options); self; self = dynamic_cast<Evaluators::CurriedOperation*>(self->fOperation)) {
		v = reduce(self->fArgument); // backwards...
		if(B_pending_value && keyword_P(v)) {
			result.push_back(std::make_pair(dynamic_cast<AST::Keyword*>(v), p));
			p = NULL;
			B_pending_value = false;
		} else {
			if(B_pending_value)
				result.push_front(std::pair<AST::Keyword*, AST::Node*>(NULL, p));
			p = v;
			B_pending_value = true;
		}
	}
	if(B_pending_value) {
		B_pending_value = false;
		result.push_front(std::pair<AST::Keyword*, AST::Node*>(NULL, p));
	}
	for(std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = result.begin(); iter != result.end(); ++iter) {
		//std::string kk = str(iter->first);
		//std::string vv = str(iter->second);
		//printf("%s %s\n", kk.c_str(), vv.c_str());
	}
	return(result);
}
AST::Node* CXXgetKeywordArgumentValue(const std::list<std::pair<AST::Keyword*, AST::Node*> >& list, AST::Keyword* key) {
	for(std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = list.begin(); iter != list.end(); ++iter)
		if(iter->first == key)
			return(iter->second);
	return(NULL);
}

}; /* end namespace Evaluators */
