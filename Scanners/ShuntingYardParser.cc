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
AST::Node* ShuntingYardParser::parse(OperatorPrecedenceList* OPL) {
	// TODO macros (especially 'let)
	std::stack<AST::Symbol*> fOperators;
	std::stack<AST::Node*> fOperands;
	// "(" is an operator. ")" is an operand, more or less.
	if(input_value == AST::intern(")")) {
		while(!fOperators.empty() && fOperators.back() != AST::intern(")")) {
			// FIXME for right associativity, leave operators with equal precedence on the stack.
			// TODO for unary operators (if there are any), only do this for other unary operators.
			AST::Symbol* op1 = fOperators.back();
			op1 fOperands.pop_back() fOperands.pop_back()
			fOperators.pop_back();
		}
		// discard fOperators.push_back(consume());
	} else if(input_value == AST::intern("(") || OPL->any_operator_P(input_value)) { /* operator */
		AST::Symbol* currentAssociativity = AST::intern("left"); // FIXME
		int currentPrecedence = input_value == AST::intern("(") ? (-1) : OPL->get_operator_precedence_and_associativity(dynamic_cast<AST::Symbol*>(input_value), currentAssociativity);
		while(!fOperators.empty() && OPL->get_operator_precedence(fOperators.back()) >= currentPrecedence) {
			if(currentAssociativity == AST::intern("right") && OPL->get_operator_precedence(fOperators.back()) == currentPrecedence)
				break;
			// FIXME for right associativity, leave operators with equal precedence on the stack.
			// TODO for unary operators (if there are any), only do this for other unary operators.
			// TODO for prefix or postfix operators, check the previous input_value, if it was an operand, then binary and postfix.
			//      otherwise (if there was an operator or none) then prefix.
			AST::Symbol* op1 = fOperators.back();
			op1 fOperands.pop_back() fOperands.pop_back()
			fOperators.pop_back();
		}
		fOperators.push_back(consume());
	} else { /* operand */
		fOperands.push_back(consume());
	}
	assert(fOperands.size() == 1);
	return(fOperands.back());
}

}; /* end namespace Scanners */
