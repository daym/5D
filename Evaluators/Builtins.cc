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

namespace Evaluators {

bool Quoter::eager_P(void) const {
	return(false);
}
AST::Node* Quoter::execute(AST::Node* argument) {
	/* there's a special case in the annotator, so this cannot happen:
	AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(argument);
	return(ref ? ref->symbol : argument);
	*/
	return(argument);
}
using namespace AST;
AST::Node* churchTrue = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
AST::Node* churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
AST::Node* internNative(bool value) {
	return(value ? churchTrue : churchFalse);
}
AST::Node* ProcedureP::execute(AST::Node* argument) {
	return(internNative(argument != NULL && (dynamic_cast<Operation*>(argument) != NULL)));
}
Conser2::Conser2(AST::Node* head) {
	this->head = head;
}
AST::Node* Conser2::execute(AST::Node* argument) {
	/* FIXME error message if it doesn't work. */
	return(cons(head, dynamic_cast<AST::Cons*>(argument)));
}
std::string Conser2::str(void) const {
	return("(cons " + (head ? head->str() : "()") + ")");
}
AST::Node* Conser::execute(AST::Node* argument) {
	return(new Conser2(argument));
}
AST::Node* ConsP::execute(AST::Node* argument) {
	bool result = dynamic_cast<AST::Cons*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* NilP::execute(AST::Node* argument) {
	bool result = argument == NULL;
	return(internNative(result));
}
using namespace Numbers;
AST::Node* SymbolP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Symbol*>(argument) != NULL || dynamic_cast<SymbolReference*>(argument) != NULL;
	return(internNative(result)); /* TODO SymbolReference? */
}
AST::Node* KeywordP::execute(AST::Node* argument) {
	bool result = dynamic_cast<Keyword*>(argument) != NULL;
	return(internNative(result));
}
#if 0
AST::Node* Int0::execute(AST::Node* argument) {
	return(internNative(0)); /* i.e. integers[0] */
}
#endif
AST::Node* StrP::execute(AST::Node* argument) {
	using namespace AST;
	bool result = dynamic_cast<Str*>(argument) != NULL;
	return(internNative(result));
}
AST::Node* HeadGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->head);
	else
		return(NULL); // FIXME proper error message!
}
AST::Node* TailGetter::execute(AST::Node* argument) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(argument);
	if(consNode)
		return(consNode->tail);
	else
		return(NULL); // FIXME proper error message!
}
AST::Node* Interner::execute(AST::Node* argument) {
	AST::Str* stringNode = dynamic_cast<AST::Str*>(argument);
	if(stringNode)
		return(AST::intern(stringNode->text.c_str()));
	else
		return(NULL);
}
AST::Node* KeywordFromStringGetter::execute(AST::Node* argument) {
	AST::Str* stringNode = dynamic_cast<AST::Str*>(argument);
	if(stringNode)
		return(AST::keywordFromString(stringNode->text.c_str()));
	else
		return(NULL);
}
AST::Node* KeywordStr::execute(AST::Node* argument) {
	AST::Keyword* keywordNode = dynamic_cast<AST::Keyword*>(argument);
	if(keywordNode)
		return(str_literal(keywordNode->name)); // TODO stop converting it back and forth and back and forth
	else
		return(NULL);
}
static std::map<AST::Symbol*, AST::Node*> cachedDynamicBuiltins;
static AST::Node* get_dynamic_builtin(AST::Symbol* symbol) {
	const char* name;
	name = symbol->name;
	if((name[0] >= '0' && name[0] <= '9') || name[0] == '-') { /* hello, number */
		long int value;
		char* endptr = NULL;
		value = strtol(name, &endptr, 10);
		if(endptr && *endptr) { /* maybe a real */
			NativeFloat value;
			std::istringstream sst(name);
			if(sst >> value)
				return(Numbers::internNative(value));
			else
				return(NULL);
		} else { /* maybe too big anyway */
			if(value == LONG_MIN || value == LONG_MAX) {
				return(symbol); // TODO allow to hook into this.
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
#define IMPLEMENT_NUMERIC_BUILTIN(N, op) \
AST::Node* N::execute(AST::Node* argument) { \
	return(new Curried ## N(NULL/*FIXME*/, argument)); \
} \
AST::Node* Curried##N::execute(AST::Node* argument) { \
	AST::Node* a = fArgument; \
	AST::Node* b = argument; \
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a); \
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b); \
	if(aInt && bInt) { \
		return(*aInt op *bInt); \
	} else { \
		Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a); \
		Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b); \
		if(aFloat && bFloat) \
			return(*aFloat op *bFloat); \
		else if(aFloat && bInt) \
			return(*aFloat op promoteToFloat(*bInt)); \
		else if(aInt && bFloat) \
			return(promoteToFloat(*aInt) op *bFloat); \
	} \
	return(operation(fallback ? fallback : AST::intern(#op), a, b)); \
}
IMPLEMENT_NUMERIC_BUILTIN(Adder, +)
IMPLEMENT_NUMERIC_BUILTIN(Subtractor, -)
IMPLEMENT_NUMERIC_BUILTIN(Multiplicator, *)
AST::Node* LEComparer::execute(AST::Node* argument) {
	return(new CurriedLEComparer(NULL/*FIXME*/, argument));
}
AST::Node* CurriedLEComparer::execute(AST::Node* argument) {
	AST::Node* a = fArgument;
	AST::Node* b = argument;
	Numbers::Int* aInt = dynamic_cast<Numbers::Int*>(a);
	Numbers::Int* bInt = dynamic_cast<Numbers::Int*>(b);
	if(aInt && bInt) {
		return(internNative(aInt->value <= bInt->value));
	} else {
		Numbers::Float* aFloat = dynamic_cast<Numbers::Float*>(a);
		Numbers::Float* bFloat = dynamic_cast<Numbers::Float*>(b);
		if(aFloat && bFloat)
			return(internNative(aFloat->value <= bFloat->value));
		else if(aFloat && bInt)
			return(internNative(aFloat->value <= bInt->value));
		else if(aInt && bFloat)
			return(internNative(aInt->value <= bFloat->value));
	}
	return(operation(fallback ? fallback : AST::intern("<="), a, b));
}

}; /* end namespace Evaluators */
