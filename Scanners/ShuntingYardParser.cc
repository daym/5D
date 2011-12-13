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
#include "Scanners/MathParser"
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

bool macro_operator_P(AST::Node* operator_) {
	return(operator_ == Symbols::Sdefine || 
	       operator_ == Symbols::Sdef || 
	       operator_ == Symbols::Sdefrec || 
	       operator_ == Symbols::Squote || 
	       operator_ == Symbols::Sleftbracket || 
	       operator_ == Symbols::Slet);
}
bool prefix_operator_P(AST::Node* operator_) {
	return(macro_operator_P(operator_));
}
ShuntingYardParser::ShuntingYardParser(void) {
}
AST::Node* MathParser::parse_abstraction(void) {
	AST::Symbol* parameter;
	if((parameter = dynamic_cast<AST::Symbol*>(input_value)) == NULL) {
		raise_error("<symbol>", str(input_value));
		return(NULL);
	} else {
		consume();
		if(EOFP() || input_value == Symbols::Srightparen || input_value == Symbols::Srightbracket || input_value == Symbols::Sautorightparen)
			raise_error("<body>", str(input_value));
		enter_abstraction(parameter);
		try {
			AST::Node* expression = parse_expression();
			leave_abstraction(parameter);
			if(expression)
				return(makeAbstraction(parameter, expression));
			else // ???
				return(makeAbstraction(parameter, NULL));
		} catch (...) {
			leave_abstraction(parameter);
			throw;
		}
	}
}
AST::Node* ShuntingYardParser::parse_value(void) {
	if(input_value == Symbols::Sbackslash) { // function abstraction
		consume();
		return(parse_abstraction());
	} else
		return(consume());
}
#define CONSUME_OPERATION { \
	AST::Symbol* op1 = fOperators.back(); \
	if(fOperands.empty()) \
		raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	AST::Node* b = fOperands.back(); \
	fOperands.pop_back(); \
	if(fOperands.empty()) \
		raise_error(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	AST::Node* a = fOperands.back(); \
	fOperands.pop_back(); \
	fOperands.push_back(AST::makeOperation(op1, a, b)); \
	fOperators.pop_back(); }
AST::Node* ShuntingYardParser::parse(OperatorPrecedenceList* OPL) {
	// TODO indentation parens
	// TODO curried operators (probably easiest to generate a symbol and put it in place instead of the second operand?)
	// TODO prefix-style operators on their own, i.e. (+)
	std::stack<AST::Symbol*> fOperators;
	std::stack<AST::Node*> fOperands;
	// "(" is an operator. ")" is an operand, more or less.
	AST::Node* previousValue = NULL;
	for(; AST::Node* value = parse_value(), value != Symbols::SlessEOFgreater; previousValue = value) {
		if(!OPL->any_operator_P(previousValue) && !OPL->any_operator_P(value)) {
			// fake previousValue Sspace value operation. Note that previousValue has already been handled in the previous iteration.
			fOperands.push_back(value);
			value = Symbols::Sspace;
		}
		if(value == Symbols::Srightparen) {
			while(!fOperators.empty() && fOperators.back() != Symbols::Sleftparen)
				CONSUME_OPERATION
		} else if(prefix_operator_P(value)) {
			while(!fOperators.empty() && prefix_operator_P(fOperators.back()))
				CONSUME_OPERATION
			// FIXME parse the macro and replace the entire thing by some hint on what to do.
			fOperators.push_back(value);
		} else if(value == Symbols::Sleftparen || OPL->any_operator_P(value)) { /* operator */
			AST::Symbol* currentAssociativity = Symbols::Sleft; // FIXME
			int currentPrecedence = value == Symbols::Sleftparen ? (-1) : OPL->get_operator_precedence_and_associativity(dynamic_cast<AST::Symbol*>(value), currentAssociativity);
			while(!fOperators.empty() && OPL->get_operator_precedence(fOperators.back()) >= currentPrecedence) {
				if(currentAssociativity == Symbols::Sright && OPL->get_operator_precedence(fOperators.back()) == currentPrecedence)
					break;
				// FIXME non-associative operators.
				// TODO for unary operators (if there are any), only do this for other unary operators.
				// TODO for prefix or postfix operators, check the previous value, if it was an operand, then binary and postfix.
				//      otherwise (if there was an operator or none) then prefix.
				CONSUME_OPERATION
			}
			fOperators.push_back(value);
		} else { /* operand */
			fOperands.push_back(value);
		}
	}
	assert(fOperands.size() == 1);
	return(fOperands.back());
}

}; /* end namespace Scanners */
