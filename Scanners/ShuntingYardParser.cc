/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <stack>
#include <string.h>
#include <deque>
#include "Scanners/ShuntingYardParser"
#include "Scanners/OperatorPrecedenceList"
#include "Values/Values"
#include "Values/Symbols"
#include "Evaluators/Builtins"

#ifdef _WIN32
/* for fmemopen used in parse_simple... */
#include "stdafx.h"
#endif
namespace Scanners {
using namespace AST;
using namespace Evaluators;

/* macros work like this: 

A. complex macros
1. They are recognized as prefix operator by prefix_operator_P (with associativity "right").
2. They are prepared as a stand-in first operand for the (ex-)macro operator by handle_unary_operator.
3. Eventually they are expanded by expand_macro.

B. simple macros ([] is the only one so far)
1. They are detected on handling operands by expand_simple_macro. These do not have a stand-in.
All these levels of indirection are in order to conserve stack space. Otherwise a sane person would prefer to use the old-style MathParser - but it doesn't scale.

*/
// keep in sync with OperatorPrecedenceList P entries.
bool prefix_operator_P(AST::NodeT operator_) {
	return(operator_ == Symbols::Squote || 
	       operator_ == Symbols::Sbackslash || 
	       operator_ == Symbols::Sdash ||
	       operator_ == Symbols::Sunarydash ||
	       (macro_operator_P(operator_) && operator_ != Symbols::Sleftbracket));
}
ShuntingYardParser::ShuntingYardParser(void) {
	bound_symbols = NULL;
	scanner = new Scanner();
}
AST::NodeT ShuntingYardParser::parse_abstraction_parameter(void) {
	AST::NodeT parameter = scanner->input_value;
	if(parameter == Symbols::Sleftparen) {
		scanner->consume();
		parameter = scanner->consume();
		scanner->consume(Symbols::Srightparen);
	} else
		scanner->consume(); /* consume parameter */
	if(!AST::get_symbol1_name(parameter)) {
		scanner->raise_error("<symbol>", str(parameter));
		return(NULL);
	} else {
		if(scanner->input_value == Symbols::SlessEOFgreater || scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::Srightbracket || scanner->input_value == Symbols::Sautorightparen)
			scanner->raise_error("<body>", str(scanner->input_value));
		// TODO enter_abstraction(parameter);
		// leave_abstraction(parameter);
		return(parameter);
	}
}
bool ShuntingYardParser::macro_standin_P(AST::NodeT op1) {
	return(!AST::get_symbol1_name(op1));
}
AST::NodeT ShuntingYardParser::parse_value(void) {
	if(scanner->input_value == Symbols::SlessEOFgreater) {
		scanner->raise_error("<parameter>", "<nothing>");
		return(NULL);
	} else if(scanner->input_value == Symbols::Sleftparen) {
		scanner->consume();
		AST::NodeT result = parse_expression(OPL, Symbols::Srightparen);
		scanner->consume();
		return(result);
	} else if(scanner->input_value == Symbols::Sautoleftparen) {
		scanner->consume();
		AST::NodeT result = parse_expression(OPL, Symbols::Sautorightparen);
		scanner->consume();
		return(result);
	} else if(macro_operator_P(scanner->input_value))
		return(expand_simple_macro(scanner->consume()));
	else
		return(parse_expression(OPL, Symbols::Sspace));
}
AST::NodeT ShuntingYardParser::parse_define_macro(AST::NodeT operator_) {
	AST::NodeT parameter = parse_value(); // this is supposed to be a symbol or so
	AST::NodeT body = parse_expression(OPL, Symbols::SlessEOFgreater); // backwards compat
	//return(AST::makeCons(operator_, AST::makeCons(parameter, NULL)));
	if(symbol_P(parameter))
		parameter = quote(parameter);
	if(abstraction_P(body))
		body = quote(body);
	return(AST::makeApplication(AST::makeApplication(operator_, parameter), body));
}
AST::NodeT ShuntingYardParser::parse_let_macro(AST::NodeT operator_ /* symbol */) {
	AST::NodeT parameter;
	if(scanner->input_value == Symbols::Sleftparen) {
		scanner->consume();
		parameter = scanner->consume();
		scanner->consume(Symbols::Srightparen);
	} else 
		parameter = scanner->consume(); // a symbol
	//parse_value(); // this is supposed to be a symbol or so
	if(scanner->input_value == Symbols::Sequal) {
		/* backwards compat */
		scanner->consume(Symbols::Sequal);
	} else
		scanner->consume(Symbols::Scolonequal);
	//AST::NodeT body = parse_value();
	AST::NodeT body = parse_expression(OPL, Symbols::Sin);
	scanner->consume(Symbols::Sin);
	return(AST::makeCons(operator_, AST::makeCons(parameter, AST::makeCons(body, NULL))));
}
AST::NodeT ShuntingYardParser::parse_import_macro(void) {
	scanner->consume(Symbols::Sleftbracket);
	AST::NodeT exports = parse_imports_macro_body();
	scanner->consume(Symbols::Srightbracket);
	scanner->consume(Symbols::Sfrom);
	AST::NodeT moduleR = parse_expression(OPL, Symbols::Sin);
	scanner->consume(Symbols::Sin);
	return(AST::makeCons(Symbols::Simport, AST::makeCons(exports, AST::makeCons(moduleR, NULL))));
}
AST::NodeT ShuntingYardParser::parse_list_macro_body(void) {
	if(scanner->input_value != Symbols::Srightbracket && scanner->input_value != Symbols::SlessEOFgreater) {
		AST::NodeT hd = parse_value();
		return(AST::makeApplication(AST::makeApplication(Symbols::Scolon, hd), parse_list_macro_body()));
	} else
		return(Symbols::Snil);
}
AST::NodeT ShuntingYardParser::parse_list_macro(void) {
	AST::NodeT result = parse_list_macro_body();
	scanner->consume(Symbols::Srightbracket);
	return(result);
}
AST::NodeT ShuntingYardParser::parse_exports_macro_body(void) { /* also used by parse_import_macro() */
	if(scanner->input_value != Symbols::Srightbracket && scanner->input_value != Symbols::SlessEOFgreater) {
		AST::NodeT a = parse_value();
		AST::NodeT hd = AST::makeApplication(AST::makeApplication(Symbols::Scomma, AST::makeApplication(Symbols::Squote, a)), a);
		return(AST::makeApplication(AST::makeApplication(Symbols::Scolon, hd), parse_exports_macro_body()));
	} else
		return(Symbols::Snil);
}
AST::NodeT ShuntingYardParser::parse_imports_macro_body(void) {
	if(scanner->input_value != Symbols::Srightbracket && scanner->input_value != Symbols::SlessEOFgreater) {
		AST::NodeT hd = parse_value(); // symbol, should be
		return(AST::makeApplication(AST::makeApplication(Symbols::Scolon, hd), parse_imports_macro_body()));
	} else
		return(Symbols::Snil);
}
AST::NodeT ShuntingYardParser::parse_exports_macro(void) {
	scanner->consume(Symbols::Sleftbracket);
	AST::NodeT result = parse_exports_macro_body();
	scanner->consume(Symbols::Srightbracket);
	return(result);
}
AST::NodeT ShuntingYardParser::parse_quote_macro(void) {
	if(scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::Sautorightparen || scanner->input_value == Symbols::SlessEOFgreater) {
		// this cannot be quoted, so just bail out, resulting in the quote function.
		return(Symbols::Squote);
	}
	AST::NodeT body = parse_value();
	return(AST::makeApplication(Symbols::Squote, body));
}
inline bool simple_macro_P(AST::NodeT value) {
	return(value == Symbols::Sleftbracket || value == Symbols::Squote);
}
AST::NodeT ShuntingYardParser::expand_simple_macro(AST::NodeT value) {
	return (value == Symbols::Sleftbracket) ? parse_list_macro() :
	       (value == Symbols::Squote) ? parse_quote_macro() :
	       (value == Symbols::Shashexports) ? parse_exports_macro() :
	       value;
}
AST::NodeT ShuntingYardParser::handle_unary_operator(AST::NodeT operator_) {
	// these will "prepare" macros by parsing the macro and representing everything but the tail (if applicable) in an AST. Later, expand_macro will fit it into the whole.
	if(operator_ == Symbols::Sbackslash) {
		AST::NodeT parameter = parse_abstraction_parameter();
		//std::string v = str(parameter);
		//printf("abstr param %s\n", v.c_str());
		return(AST::makeCons(operator_, AST::makeCons(parameter, NULL)));
	} else if(operator_ == Symbols::Squote) {
		if(scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::Sautorightparen || scanner->input_value == Symbols::SlessEOFgreater) {
			// these cannot be quoted.
			return(Symbols::Squote);
		} else
			return(AST::makeCons(Symbols::Squote, NULL));
	} else if(operator_ == Symbols::Slet) {
		return(parse_let_macro(operator_));
	} else if(operator_ == Symbols::Sletexclam) {
		return(parse_let_macro(operator_));
	} else if(operator_ == Symbols::Simport) {
		return(parse_import_macro());
	} else if(operator_ == Symbols::Sunarydash) {
		return(AST::makeCons(Symbols::Sunarydash, NULL));
	}
	return(operator_);
}
static AST::NodeT macro_standin_operator(AST::NodeT op1) {
	return Evaluators::cons_P(op1) ? AST::get_cons_head(op1) : NULL;
}
// [a b c d e f g], F, body => \a \b \c \d \e \f \g body) (F a) (F b) (F c) ...
static AST::NodeT closeMany(AST::NodeT symbols, AST::NodeT dependency, AST::NodeT suffix) {
	// note that #symbols in unevaluated (save for the macro expansions) as you would expect.
	//return(Evaluators::close(parameter, replacement, suffix));
	// (: a (: b (:c n))) => []
	AST::NodeT t;
	if(AST::application_P(symbols) && AST::application_P((t = AST::get_application_operator(symbols))) && AST::get_application_operator(t) == Symbols::Scolon) {
		AST::NodeT a = AST::get_application_operand(t); // should be a symbol
		assert(AST::get_symbol1_name(a));
		AST::NodeT r = AST::get_application_operand(symbols);
		// note: possible module access.
		return(Evaluators::close(a, AST::makeApplication(dependency, Evaluators::quote(a)), closeMany(r, dependency, suffix)));
	} else { /* assume nil. */
		assert(symbols == Symbols::Snil);
		return(suffix);
	}
}
AST::NodeT ShuntingYardParser::expand_macro(AST::NodeT op1, AST::NodeT suffix) {
	if(!Evaluators::cons_P(op1))
		abort();
	AST::NodeT operator_ = get_cons_head(op1);
	if(operator_ == Symbols::Sbackslash) {
		AST::NodeT parameter = AST::get_cons_head(Evaluators::evaluateToCons(get_cons_tail(op1)));
		return(AST::makeAbstraction(parameter, suffix));
	} else if(operator_ == Symbols::Squote) {
		return(AST::makeApplication(Symbols::Squote, suffix));
	} else if(operator_ == Symbols::Slet) {
		assert(AST::get_cons_tail(op1));
		AST::NodeT c2 = Evaluators::evaluateToCons(AST::get_cons_tail(op1));
		AST::NodeT parameter = AST::get_cons_head(c2);
		assert(AST::get_symbol1_name(parameter));
		assert(AST::get_cons_tail(c2));
		AST::NodeT c3 = Evaluators::evaluateToCons(AST::get_cons_tail(c2));
		AST::NodeT replacement = AST::get_cons_head(c3);
		return(Evaluators::close(parameter, replacement, suffix));
	} else if(operator_ == Symbols::Sletexclam) {
		/* let! y := a in B =>
		   (;) a (\y B) */
		assert(AST::get_cons_tail(op1));
		AST::NodeT c2 = Evaluators::evaluateToCons(AST::get_cons_tail(op1));
		AST::NodeT parameter = AST::get_cons_head(c2);
		assert(AST::get_symbol1_name(parameter));
		assert(AST::get_cons_tail(c2));
		AST::NodeT c3 = Evaluators::evaluateToCons(AST::get_cons_tail(c2));
		AST::NodeT replacement = AST::get_cons_head(c3);
		return(AST::makeOperation(Symbols::Ssemicolon, replacement, AST::makeAbstraction(parameter, suffix))); /* NOTE variable capture... */
	} else if(operator_ == Symbols::Simport) {
		assert(AST::get_cons_tail(op1));
		AST::NodeT c2 = Evaluators::evaluateToCons(AST::get_cons_tail(op1));
		AST::NodeT symbols = AST::get_cons_head(c2);
		// TODO eval the list (using a very limited eval) assert(AST::get_symbol1_name(parameter));
		assert(AST::get_cons_tail(c2));
		AST::NodeT c3 = Evaluators::evaluateToCons(AST::get_cons_tail(c2));
		AST::NodeT dependency = AST::get_cons_head(c3);
		AST::NodeT r = closeMany(symbols, dependency, suffix);
		std::string s;
		s = Evaluators::str(r);
		return(r);
	} else if(operator_ == Symbols::Sunarydash) {
		return(AST::makeOperation(Symbols::Sdash, Symbols::Szero, suffix));
	} else {
		scanner->raise_error("<macro-body>", str(op1));
		return(NULL);
	}
}
AST::NodeT makeOperationDot(AST::NodeT op, AST::NodeT a, AST::NodeT b) {
	if(op == Symbols::Sdot)
		return(AST::makeOperation(op, a, quote(b)));
	else
		return(AST::makeOperation(op, a, b));
}
#define CONSUME_OPERATION { \
	AST::NodeT op1 = fOperators.top(); \
	/*std::cout << str(op1) << std::endl;*/ \
	bool bUnary = macro_standin_P(op1); \
	AST::NodeT b = NULL; \
	if(!bUnary) { \
		if(fOperands.empty()) \
			scanner->raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
		b = fOperands.top(); \
		fOperands.pop(); \
	} \
	if(fOperands.empty()) \
		scanner->raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	AST::NodeT a = fOperands.top(); \
	fOperands.pop(); \
	fOperands.push(bUnary ? expand_macro(op1, a) : makeOperationDot(op1, a, b)); \
	fOperators.pop(); }
bool ShuntingYardParser::any_operator_P(AST::NodeT node) {
	// fake '(' and 'auto('
	if(node == Symbols::Sleftparen || node == Symbols::Sautoleftparen || node == Symbols::Srightparen || node == Symbols::Sautorightparen)
		return(true);
	else
		return(OPL->any_operator_P(node));
}
int ShuntingYardParser::get_operator_precedence_and_associativity(AST::NodeT node, AST::NodeT& outAssociativity) {
	AST::Cons* c = Evaluators::evaluateToCons(node);
	if(c) // macro-like operators have their operator symbol as the head
		node = c->head;
	assert(AST::get_symbol1_name(node) != NULL);
	outAssociativity = Symbols::Sright;
	return(OPL->get_operator_precedence_and_associativity(node, outAssociativity));
}
int ShuntingYardParser::get_operator_precedence(AST::NodeT node) {
	AST::NodeT associativity = NULL;
	return(get_operator_precedence_and_associativity(node, associativity));
}
#define SCOPERANDS \
	unsigned int prevOperandCount = fOperandCounts.top(); \
	if(!fOperators.empty() && fOperators.top() != Symbols::Sleftparen && fOperators.top() != Symbols::Sautoleftparen && (!macro_standin_P(fOperators.top()) || macro_standin_operator(fOperators.top()) == Symbols::Sunarydash) && fOperands.size() <= prevOperandCount) { /* well, there has been nothing interesting in the parentesized expression yet. */ \
		AST::NodeT operand = fOperators.top(); \
		if(macro_standin_P(operand)) \
			operand = macro_standin_operator(operand); \
		if(operand == Symbols::Sunarydash) /* long standing tradition */ \
			operand = Symbols::Sdash; \
		fOperands.push(operand); \
		fOperators.pop(); \
	} else \

AST::NodeT ShuntingYardParser::parse_expression(OperatorPrecedenceList* OPL, NodeT terminator) {
	bool oldIndentationHonoring = false;
	try {
		oldIndentationHonoring = scanner->setHonorIndentation(true);
	// TODO curried operators (probably easiest to generate a symbol and put it in place instead of the second operand?)
	std::stack<AST::NodeT, std::deque<AST::NodeT, gc_allocator<AST::NodeT> > > fOperators;
	std::stack<AST::NodeT, std::deque<AST::NodeT, gc_allocator<AST::NodeT> >  > fOperands;
	std::stack<unsigned int> fOperandCounts; // for paren operand counting
	fOperandCounts.push(0U);
	AST::NodeT previousValue = Symbols::Sleftparen;
	AST::NodeT value;
	this->OPL = OPL;
	for(; value = scanner->input_value, (value != terminator || fOperandCounts.size() > 1) && value != Symbols::SlessEOFgreater; previousValue = value) {
		if(previousValue != Symbols::Sspace && 
		(!OPL->any_operator_P(previousValue) && 
		 previousValue != Symbols::Sleftparen && 
		 previousValue != Symbols::Sautoleftparen && 
		 ((!OPL->any_operator_P(value) || (prefix_operator_P(value) && value != Symbols::Sdash) || value == Symbols::Sbackslash) &&
		   value != Symbols::Srightparen && 
		   value != Symbols::Sautorightparen))) { // two consecutive non-operators: function application.
			// not "x)" !
			// fake previousValue Sspace value operation. Note that previousValue has already been handled in the previous iteration.
			//fOperands.push(expand_simple_macro(value));
			value = Symbols::Sspace;
			// do not consume
			if(value == terminator && fOperandCounts.size() <= 1)
				break;
		} else {
			//if(any_operator_P(previousValue) && previousValue != Symbols::Sleftparen && previousValue != Symbols::Srightparen) {
			//// on the other hand, if both are, we have an unary operator - or at least something that looks like an unary operator.
			//// we could do special handling here (i.e. rename "-" to "unary-" or whatever)
			scanner->consume();
		}
		if(value == Symbols::Srightparen || value == Symbols::Sautorightparen) { // handle grouping, part 2
			SCOPERANDS while(!fOperators.empty() && fOperators.top() != Symbols::Sleftparen && fOperators.top() != Symbols::Sautoleftparen)
				CONSUME_OPERATION
			fOperandCounts.pop();
			if(fOperators.empty())
				scanner->raise_error("<stuff>", str(value)); 
			else if(value == Symbols::Srightparen && fOperators.top() != Symbols::Sleftparen)
				scanner->raise_error(std::string("close-") + str(fOperators.top()), str(value));
			else if(value == Symbols::Sautorightparen && fOperators.top() != Symbols::Sautoleftparen)
				scanner->raise_error(std::string("close-") + str(fOperators.top()), str(value));
			fOperators.pop(); // "("
		} else if(prefix_operator_P(value) && any_operator_P(previousValue) && previousValue != Symbols::Srightparen && previousValue != Symbols::Sautorightparen) { // unary prefix operator, assumed to all be right-associative.
			if(value == Symbols::Sdash)
				value = Symbols::Sunarydash;
			int currentPrecedence = get_operator_precedence(value);
			SCOPERANDS while(!fOperators.empty() && prefix_operator_P(fOperators.top()) && currentPrecedence < get_operator_precedence(fOperators.top()))
				CONSUME_OPERATION
			//printf("prefix pushop %s\n", str(value).c_str());
			fOperators.push(handle_unary_operator(value));
		} else if(value == Symbols::Sleftparen || value == Symbols::Sautoleftparen) { // handle grouping, part 1
			//printf("pushop %s\n", str(value).c_str());
			fOperators.push(value);
			fOperandCounts.push(fOperands.size());
		} else if(any_operator_P(value) && (fOperators.empty() || 1 /*|| macro_standin_operator(fOperators.top()) != Symbols::Squote */ || value == Symbols::Sspace)) { /* operator */
			// TODO check for postfix operators here.
			NodeT currentAssociativity = Symbols::Sright; // TODO
			// note that prefix associativity is right associativity.
			int currentPrecedence = get_operator_precedence_and_associativity(value, currentAssociativity);
			SCOPERANDS while(!fOperators.empty() && currentPrecedence <= get_operator_precedence(fOperators.top())) {
				if(currentAssociativity != Symbols::Sleft && currentPrecedence == get_operator_precedence(fOperators.top()))
					break;
				// FIXME non-associative operators.
				// TODO for prefix or postfix operators, check the previous value, if it was an operand, then binary and postfix.
				//      otherwise (if there was an operator or none) then prefix.
				CONSUME_OPERATION
			}
			//printf("pushop %s\n", str(value).c_str());
			fOperators.push(value);
		} else { /* operand */
			fOperands.push(expand_simple_macro(value));
		}
	}
	//printf("len operators %d %s\n", (int) fOperators.size(), fOperators.size() ? str(fOperators.top()).c_str() : "");
	//printf("len operands %d %s\n", (int) fOperands.size(), fOperands.size() ? str(fOperands.top()).c_str() : "");
	if(value != terminator)
		scanner->raise_error(str(terminator), str(value));
	SCOPERANDS while(!fOperators.empty())
		CONSUME_OPERATION
	if(fOperands.empty())
		scanner->raise_error("<something>", "<nothing>");
	if(fOperands.size() != 1) {
		/* Example: [*2] */
		scanner->raise_error("<nothing>", "<something>");
	}
	scanner->setHonorIndentation(oldIndentationHonoring);
	return(fOperands.top());
	} catch(...) {
		scanner->setHonorIndentation(oldIndentationHonoring);
		throw;
	}
}
AST::NodeT ShuntingYardParser::parse(OperatorPrecedenceList* OPL, NodeT terminator) {
	return(parse_expression(OPL, terminator));
}
void ShuntingYardParser::enter_abstraction(NodeT name) {
	bound_symbols = AST::makeCons(name, bound_symbols);
}
void ShuntingYardParser::leave_abstraction(NodeT name) {
	assert(!AST::nil_P(bound_symbols) && AST::get_cons_head(bound_symbols) == name);
	AST::NodeT n = AST::get_cons_tail(bound_symbols);
	AST::set_cons_tail(bound_symbols, NULL);
	bound_symbols = (AST::Cons*) n;
}
void ShuntingYardParser::push(FILE* input_file, int line_number, const char* input_name) {
	// TODO maybe just replace the entire scanner (making sure to copy input_value over).
	scanner->push(input_file, line_number, input_name);
	scanner->consume();
}
void ShuntingYardParser::pop(void) {
	// TODO maybe just replace the entire scanner (making sure to copy input_value over).
	scanner->pop();
}
int ShuntingYardParser::get_position(void) const {
	return(scanner->get_position());
}

}; /* end namespace Scanners */
