/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
// TODO define def defrec auto(
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <stack>
#include <string.h>
#include "Scanners/ShuntingYardParser"
#include "Scanners/OperatorPrecedenceList"
#include "AST/Symbol"
#include "AST/Symbols"
#include "AST/AST"
#include "AST/Keyword"
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
bool prefix_operator_P(AST::Node* operator_) {
	return(operator_ == Symbols::Squote || operator_ == Symbols::Sbackslash || (macro_operator_P(operator_) && operator_ != Symbols::Sleftbracket));
}
ShuntingYardParser::ShuntingYardParser(void) {
	bound_symbols = NULL;
	scanner = new Scanner();
}
AST::Node* ShuntingYardParser::parse_abstraction_parameter(void) {
	AST::Symbol* parameter;
	if((parameter = dynamic_cast<AST::Symbol*>(scanner->input_value)) == NULL) {
		scanner->raise_error("<symbol>", str(scanner->input_value));
		return(NULL);
	} else {
		scanner->consume(); /* consume parameter */
		if(scanner->input_value == Symbols::SlessEOFgreater || scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::Srightbracket || scanner->input_value == Symbols::Sautorightparen)
			scanner->raise_error("<body>", str(scanner->input_value));
		// TODO enter_abstraction(parameter);
		// leave_abstraction(parameter);
		return(parameter);
	}
}
bool ShuntingYardParser::macro_standin_P(AST::Node* op1) {
	return(dynamic_cast<AST::Symbol*>(op1) == NULL);
}
AST::Node* ShuntingYardParser::parse_value(void) {
	// TODO maybe at least allow other macros?
	if(scanner->input_value == Symbols::SlessEOFgreater) {
		scanner->raise_error("<parameter>", "<nothing>");
		return(NULL);
	} else if(scanner->input_value == Symbols::Sleftparen) {
		scanner->consume();
		AST::Node* result = parse_expression(OPL, Symbols::Srightparen);
		scanner->consume();
		return(result);
	} else if(scanner->input_value == Symbols::Sautoleftparen) {
		scanner->consume();
		AST::Node* result = parse_expression(OPL, Symbols::Sautorightparen);
		scanner->consume();
		return(result);
	} else
		return(scanner->consume());
}
AST::Node* ShuntingYardParser::parse_let_macro(void) {
	AST::Node* parameter = parse_value(); // this is supposed to be a symbol or so
	scanner->consume(Symbols::Sequal);
	AST::Node* body = parse_value();
	scanner->consume(Symbols::Sin);
	return(AST::makeCons(Symbols::Slet, AST::makeCons(parameter, AST::makeCons(body, NULL))));
}
AST::Node* ShuntingYardParser::parse_list_macro(void) {
	AST::Cons* root = NULL;
	AST::Cons* tail = NULL;
	while(scanner->input_value != Symbols::Srightbracket && scanner->input_value != Symbols::SlessEOFgreater) {
		//value = parse_value(), value != Symbols::Srightbracket && value != Symbols::SlessEOFgreater) {
		AST::Cons* n = AST::makeCons(parse_value(), NULL);
		if(tail)
			tail->tail = n;
		else
			root = n;
		tail = n;
	}
	scanner->consume(Symbols::Srightbracket);
	return(root);
}
AST::Node* ShuntingYardParser::expand_simple_macro(AST::Node* value) {
	return (value == Symbols::Sleftbracket) ? parse_list_macro() : value;
}
AST::Node* ShuntingYardParser::handle_unary_operator(AST::Node* operator_) {
	// these will "prepare" macros by parsing the macro and representing everything but the tail (if applicable) in an AST. Later, expand_macro will fit it into the whole.
	if(operator_ == Symbols::Sbackslash) {
		return(AST::makeCons(operator_, AST::makeCons(parse_abstraction_parameter(), NULL)));
	} else if(operator_ == Symbols::Squote) {
		return(AST::makeCons(operator_, NULL));
	} else if(operator_ == Symbols::Slet) {
		return(parse_let_macro());
	}
	// the remaining operators are:
	//    (define and similar)
	return(operator_);
}
AST::Node* ShuntingYardParser::expand_macro(AST::Node* op1, AST::Node* suffix) {
	AST::Cons* consOp1 = dynamic_cast<AST::Cons*>(op1);
	if(consOp1 == NULL)
		abort();
	AST::Node* operator_ = consOp1->head;
	if(operator_ == Symbols::Sbackslash) {
		assert(consOp1->tail);
		AST::Symbol* parameter = dynamic_cast<AST::Symbol*>(Evaluators::evaluateToCons(consOp1->tail)->head);
		return(AST::makeAbstraction(parameter, suffix));
	} else if(operator_ == Symbols::Squote) {
		return(AST::makeApplication(operator_, suffix));
	} else if(operator_ == Symbols::Slet) {
		assert(consOp1->tail);
		AST::Cons* c2 = Evaluators::evaluateToCons(consOp1->tail);
		AST::Symbol* parameter = dynamic_cast<AST::Symbol*>(c2->head);
		assert(parameter);
		assert(c2->tail);
		AST::Cons* c3 = Evaluators::evaluateToCons(c2->tail);
		AST::Node* replacement = c3->head;
		return(Evaluators::close(parameter, replacement, suffix));
	} else {
		scanner->raise_error("<macro-body>", str(op1));
		return(NULL);
	}
}
#define CONSUME_OPERATION { \
	AST::Node* op1 = fOperators.top(); \
	/*std::cout << str(op1) << std::endl;*/ \
	bool bUnary = macro_standin_P(op1); \
	AST::Node* b = NULL; \
	if(!bUnary) { \
		if(fOperands.empty()) \
			scanner->raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
		b = fOperands.top(); \
		fOperands.pop(); \
	} \
	if(fOperands.empty()) \
		scanner->raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	AST::Node* a = fOperands.top(); \
	fOperands.pop(); \
	fOperands.push(bUnary ? expand_macro(op1, a) : AST::makeOperation(op1, a, b)); \
	fOperators.pop(); }
bool ShuntingYardParser::any_operator_P(AST::Node* node) {
	// fake '(' and 'auto('
	if(node == Symbols::Sleftparen || node == Symbols::Sautoleftparen || node == Symbols::Srightparen || node == Symbols::Sautorightparen)
		return(true);
	else
		return(OPL->any_operator_P(node));
}
int ShuntingYardParser::get_operator_precedence_and_associativity(AST::Node* node, AST::Symbol*& outAssociativity) {
	AST::Cons* c = Evaluators::evaluateToCons(node);
	if(c) // macro-like operators have their operator symbol as the head
		node = c->head;
	assert(dynamic_cast<AST::Symbol*>(node) != NULL);
	outAssociativity = Symbols::Sright;
	return(OPL->get_operator_precedence_and_associativity((AST::Symbol*) node, outAssociativity));
}
int ShuntingYardParser::get_operator_precedence(AST::Node* node) {
	AST::Symbol* associativity = NULL;
	return(get_operator_precedence_and_associativity(node, associativity));
}
static bool second_paren_P(std::stack<AST::Node*>& stack) {
	if(stack.size() == 0)
		return(false);
	AST::Node* t = stack.top();
	if(t == Symbols::Sleftparen || t == Symbols::Sautoleftparen)
		return(false);
	else if(stack.size() == 1)
		return(true);
	stack.pop();
	AST::Node* result = stack.top();
	stack.push(t);
	return(result == Symbols::Sleftparen || result == Symbols::Sautoleftparen);
}
#define SCOPERANDS \
	if(fOperands.empty() && second_paren_P(fOperators)) { \
		fOperands.push(fOperators.top()); \
		fOperators.pop(); \
	} else \

AST::Node* ShuntingYardParser::parse_expression(OperatorPrecedenceList* OPL, AST::Symbol* terminator) {
	// TODO indentation parens
	// TODO curried operators (probably easiest to generate a symbol and put it in place instead of the second operand?)
	// TODO prefix-style operators on their own, i.e. (+)
	// TODO prefix-style lambda.
	std::stack<AST::Node*> fOperators;
	std::stack<AST::Node*> fOperands;
	// "(" is an operator. ")" is an operand, more or less.
	AST::Node* previousValue = Symbols::Sleftparen;
	AST::Node* value;
	AST::Node* originalValue;
	bool bNeedOriginalPush = false;
	this->OPL = OPL;
	for(; value = scanner->input_value, value != terminator && value != Symbols::SlessEOFgreater; previousValue = value) {
		//printf("read %s\n", str(value).c_str());
		scanner->consume();
		bNeedOriginalPush = false;
		if((!any_operator_P(previousValue) || previousValue == Symbols::Srightparen || previousValue == Symbols::Sautorightparen || previousValue == Symbols::Sspace /*|| previousValue == Symbols::Sbackslash*/) && (!any_operator_P(value) || value == Symbols::Sbackslash)) {
			// fake previousValue Sspace value operation. Note that previousValue has already been handled in the previous iteration.
			//fOperands.push(expand_simple_macro(value));
			originalValue = value;
			bNeedOriginalPush = true;
			value = Symbols::Sspace;
		} else if(any_operator_P(previousValue) && previousValue != Symbols::Sleftparen && previousValue != Symbols::Srightparen) {
			// on the other hand, if both are, we have an unary operator - or at least something that looks like an unary operator.
			// we could do special handling here (i.e. rename "-" to "unary-" or whatever)
		}
		if(value == Symbols::Srightparen || value == Symbols::Sautorightparen) {
			SCOPERANDS while(!fOperators.empty() && fOperators.top() != Symbols::Sleftparen && fOperators.top() != Symbols::Sautoleftparen)
				CONSUME_OPERATION
			if(value == Symbols::Srightparen && fOperators.top() != Symbols::Sleftparen)
				scanner->raise_error(str(Symbols::Sleftparen), str(fOperators.top()));
			else if(value == Symbols::Sautorightparen && fOperators.top() != Symbols::Sautoleftparen)
				scanner->raise_error(str(Symbols::Sautoleftparen), str(fOperators.top()));
			fOperators.pop(); // "("
		} else if(prefix_operator_P(value)) { // assumed to all be right-associative.
			int currentPrecedence = get_operator_precedence(value);
			SCOPERANDS while(!fOperators.empty() && prefix_operator_P(fOperators.top()) && currentPrecedence < get_operator_precedence(fOperators.top()))
				CONSUME_OPERATION
			fOperators.push(handle_unary_operator(value));
		} else if(value == Symbols::Sleftparen || value == Symbols::Sautoleftparen || any_operator_P(value)) { /* operator */ // FIXME
			AST::Symbol* currentAssociativity = Symbols::Sright; // FIXME
			// note that prefix associativity is right associativity.
			int currentPrecedence = value == Symbols::Sleftparen ? (-1) : value == Symbols::Sautoleftparen ? (-1) : get_operator_precedence_and_associativity(value, currentAssociativity);
			SCOPERANDS while(!fOperators.empty() && currentPrecedence <= get_operator_precedence(fOperators.top())) {
				if(currentAssociativity != Symbols::Sleft && currentPrecedence == get_operator_precedence(fOperators.top()))
					break;
				// FIXME non-associative operators.
				// TODO for unary operators (if there are any), only do this for other unary operators.
				// TODO for prefix or postfix operators, check the previous value, if it was an operand, then binary and postfix.
				//      otherwise (if there was an operator or none) then prefix.
				CONSUME_OPERATION
			}
			fOperators.push(value);
		} else { /* operand */
			fOperands.push(expand_simple_macro(value));
		}
		if(bNeedOriginalPush)
			fOperands.push(expand_simple_macro(originalValue));
	}
	if(value != terminator)
		scanner->raise_error(str(terminator), str(value));
		while(!fOperators.empty())
			CONSUME_OPERATION
	assert(fOperands.size() == 1);
	return(fOperands.top());
}
AST::Node* ShuntingYardParser::parse(OperatorPrecedenceList* OPL, AST::Symbol* terminator) {
	return(parse_expression(OPL, terminator));
}
void ShuntingYardParser::enter_abstraction(AST::Symbol* name) {
	bound_symbols = AST::makeCons(name, bound_symbols);
}
void ShuntingYardParser::leave_abstraction(AST::Symbol* name) {
	assert(bound_symbols && dynamic_cast<AST::Symbol*>(bound_symbols->head) == name);
	AST::Node* n = bound_symbols->tail;
	bound_symbols->tail = NULL;
	bound_symbols = (AST::Cons*) n;
}
void ShuntingYardParser::push(FILE* input_file, int line_number) {
	// TODO maybe just replace the entire scanner (making sure to copy input_value over).
	scanner->push(input_file, line_number);
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
