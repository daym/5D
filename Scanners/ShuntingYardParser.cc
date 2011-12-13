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

bool prefix_operator_P(AST::Node* operator_) {
	return(macro_operator_P(operator_));
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
		if(scanner->input_value == Symbols::SlessEOFgreater || scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::Srightbracket /* || scanner->input_value == Symbols::Sautorightparen*/)
			scanner->raise_error("<body>", str(scanner->input_value));
		// TODO enter_abstraction(parameter);
		// leave_abstraction(parameter);
		return(parameter);
	}
}
bool ShuntingYardParser::macro_standin_P(AST::Node* op1) {
	return(dynamic_cast<AST::Symbol*>(op1) == NULL);
}
AST::Node* ShuntingYardParser::expand_macro(AST::Node* op1, AST::Node* suffix) {
	// FIXME
	printf("macro\n");
	return(suffix);
}
#define CONSUME_OPERATION { \
	AST::Node* op1 = fOperators.top(); \
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
AST::Node* ShuntingYardParser::handle_unary_operator(AST::Node* operator_) {
	// FIXME parse the macro in value and replace the entire thing by some hint on what to do.
	// the operators are:
	//    lambda
	//    let
	//    (define and similar)
	//    '
	//    [
	return(operator_);
}
bool ShuntingYardParser::any_operator_P(AST::Node* node) {
	return(OPL->any_operator_P(node));
}
int ShuntingYardParser::get_operator_precedence_and_associativity(AST::Node* node, AST::Symbol*& outAssociativity) {
	assert(dynamic_cast<AST::Symbol*>(node) != NULL);
	return(OPL->get_operator_precedence_and_associativity((AST::Symbol*) node, outAssociativity));
}
int ShuntingYardParser::get_operator_precedence(AST::Node* node) {
	AST::Symbol* associativity = NULL;
	return(get_operator_precedence_and_associativity(node, associativity));
}
AST::Node* ShuntingYardParser::parse_expression(OperatorPrecedenceList* OPL, AST::Symbol* terminator) {
	// TODO indentation parens
	// TODO curried operators (probably easiest to generate a symbol and put it in place instead of the second operand?)
	// TODO prefix-style operators on their own, i.e. (+)
	// TODO prefix-style lambda.
	std::stack<AST::Node*> fOperators;
	std::stack<AST::Node*> fOperands;
	// "(" is an operator. ")" is an operand, more or less.
	AST::Node* previousValue = NULL;
	AST::Node* value;
	this->OPL = OPL;
	for(; value = parse_value(), value != terminator; previousValue = value) {
		if(!any_operator_P(previousValue) && !any_operator_P(value)) {
			// fake previousValue Sspace value operation. Note that previousValue has already been handled in the previous iteration.
			fOperands.push(value);
			value = Symbols::Sspace;
		}
		if(value == Symbols::Srightparen) {
			while(!fOperators.empty() && fOperators.top() != Symbols::Sleftparen)
				CONSUME_OPERATION
		} else if(prefix_operator_P(value)) {
			while(!fOperators.empty() && prefix_operator_P(fOperators.top()))
				CONSUME_OPERATION
			;
			fOperators.push(handle_unary_operator(value));
		} else if(value == Symbols::Sleftparen || any_operator_P(value)) { /* operator */
			AST::Symbol* currentAssociativity = Symbols::Sleft; // FIXME
			// note that prefix associativity is right associativity.
			int currentPrecedence = value == Symbols::Sleftparen ? (-1) : get_operator_precedence_and_associativity(value, currentAssociativity);
			while(!fOperators.empty() && get_operator_precedence(fOperators.top()) >= currentPrecedence) {
				if(currentAssociativity == Symbols::Sright && get_operator_precedence(fOperators.top()) == currentPrecedence)
					break;
				// FIXME non-associative operators.
				// TODO for unary operators (if there are any), only do this for other unary operators.
				// TODO for prefix or postfix operators, check the previous value, if it was an operand, then binary and postfix.
				//      otherwise (if there was an operator or none) then prefix.
				CONSUME_OPERATION
			}
			fOperators.push(value);
		} else { /* operand */
			fOperands.push(value);
		}
	}
	assert(fOperands.size() == 1);
	return(fOperands.top());
}
AST::Node* ShuntingYardParser::parse(OperatorPrecedenceList* OPL) {
	return(parse_expression(OPL, Symbols::SlessEOFgreater));
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

}; /* end namespace Scanners */
