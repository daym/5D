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
AST::Node* ShuntingYardParser::parse(OperatorPrecedenceList* OPL) {
	// TODO macros (especially 'let)
	// TODO indentation parens
	// TODO curried operators (probably easiest to generate a symbol and put it in place instead of the second operand?)
	// TODO prefix-style operators on their own, i.e. (+)
	// TODO function application
	std::stack<AST::Symbol*> fOperators;
	std::stack<AST::Node*> fOperands;
	// "(" is an operator. ")" is an operand, more or less.
	while(AST::Node* value = parse_value(), value != Symbols::SlessEOFgreater) {
		if(value == Symbols::Srightparen) {
			while(!fOperators.empty() && fOperators.back() != Symbols::Sleftparen) {
				AST::Symbol* op1 = fOperators.back();
				if(fOperands.empty())
					throw Scanners::ParseException(std::string("not enough operands for operator (") + str(op1) + std::string(")"));
				AST::Node* b = fOperands.back();
				fOperands.pop_back();
				if(fOperands.empty())
					throw Scanners::ParseException(std::string("not enough operands for operator (") + str(op1) + std::string(")"));
				AST::Node* a = fOperands.back();
				fOperands.pop_back();
				fOperands.push_back(AST::makeOperation(op1, a, b));
				fOperators.pop_back();
			}
			// discard fOperators.push_back(value);
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
				AST::Symbol* op1 = fOperators.back();
				if(fOperands.empty())
					throw Scanners::ParseException(std::string("not enough operands for operator (") + str(op1) + std::string(")"));
				AST::Node* b = fOperands.back();
				fOperands.pop_back();
				if(fOperands.empty())
					throw Scanners::ParseException(std::string("not enough operands for operator (") + str(op1) + std::string(")"));
				AST::Node* a = fOperands.back();
				fOperands.pop_back();
				fOperands.push_back(AST::makeOperation(op1, a, b));
				fOperators.pop_back();
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
