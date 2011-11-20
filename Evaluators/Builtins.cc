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

using namespace AST;
AST::Node* churchTrue = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f t))", NULL));
AST::Node* churchFalse = Evaluators::annotate(Scanners::MathParser::parse_simple("(\\t (\\f f))", NULL));
AST::Node* internNative(bool value) {
	return(value ? churchTrue : churchFalse);
}
using namespace Numbers;
#if 0
AST::Node* Int0::execute(AST::Node* argument) {
	return(internNative(0)); /* i.e. integers[0] */
}
#endif
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
	return(new Curried ## N(this, NULL/*FIXME*/, argument)); \
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
	return(makeOperation(AST::intern(#op), a, b)); \
}
#define IMPLEMENT_BINARY_BUILTIN(N, op, fn) \
AST::Node* N::execute(AST::Node* argument) { \
	return(new Curried ## N(this, NULL/*FIXME*/, argument)); \
} \
AST::Node* Curried##N::execute(AST::Node* argument) { \
	AST::Node* a = fArgument; \
	AST::Node* b = argument; \
	return(fn(a, b)); \
}
IMPLEMENT_NUMERIC_BUILTIN(Adder, +)
IMPLEMENT_NUMERIC_BUILTIN(Subtractor, -)
IMPLEMENT_NUMERIC_BUILTIN(Multiplicator, *)
IMPLEMENT_NUMERIC_BUILTIN(LEComparer, <=)
IMPLEMENT_BINARY_BUILTIN(Conser, :, makeCons)

}; /* end namespace Evaluators */
