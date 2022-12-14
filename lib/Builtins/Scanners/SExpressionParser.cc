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
#include "Scanners/SExpressionParser"
#include "Values/Values"
#include "Values/Symbols"
#include "Evaluators/Builtins"
#ifdef _WIN32
/* for fmemopen used in parse_simple... */
#include "stdafx.h"
#endif
namespace Scanners {
using namespace Evaluators;
using namespace Values;

SExpressionParser::SExpressionParser(void) {
	scanner = new Scanner();
}
#define CONSUME_OPERATION { \
	Symbol* op1 = fOperators.back(); \
	if(fOperands.empty()) \
		scanner->raiseError(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	NodeT b = fOperands.back(); \
	fOperands.pop_back(); \
	if(fOperands.empty()) \
		scanner->raiseError(std::string("<") + str(op1) + std::string("-operands>"), "<nothing>"); \
	NodeT a = fOperands.back(); \
	fOperands.pop_back(); \
	fOperands.push_back(makeOperation(op1, a, b)); \
	fOperators.pop_back(); }
NodeT SExpressionParser::parse_S_list_body(void) {
	if(scanner->input_value == Symbols::Srightparen || scanner->input_value == Symbols::SlessEOFgreater)
		return(NULL);
	else {
		NodeT head;
		head = parse_S_Expression();
		return(makeCons(head, parse_S_list_body()));
	}
}
NodeT SExpressionParser::parse_S_list(bool B_consume_closing_brace) {
	try {
		NodeT result = NULL;
		scanner->consume(Symbols::Sleftparen);
		result = parse_S_list_body();
		if(B_consume_closing_brace)
			scanner->consume(Symbols::Srightparen);
		return(result);
	} catch(...) {
		throw;
	}
}
NodeT SExpressionParser::parse_S_Expression(void) {
	try {
		NodeT result;
		/* TODO do this without tokenizing? How? */
		if(scanner->input_value == Symbols::Sleftparen) {
			result = parse_S_list(true);
		} else if(get_symbol1_name(scanner->input_value) != NULL) {
			result = scanner->consume(); // & whitespace.
		} else {
			/* numbers, strings */
			if(scanner->input_value)
				result = scanner->consume();
			else {
				scanner->raiseError("<S_Expression>", "<junk>");
				result = NULL;
			}
		}
		return(result);
	} catch(...) {
		throw;
	}
}
NodeT SExpressionParser::parse(NodeT terminator) {
	NodeT result = parse_S_Expression();
	if(scanner->input_value != terminator)
		scanner->raiseError(str(terminator), str(scanner->input_value));
	return(result);
}
void SExpressionParser::push(FILE* input_file, int line_number, const char* input_name) {
	// TODO maybe just replace the entire scanner (making sure to copy input_value over).
	scanner->push(input_file, line_number, input_name);
	scanner->consume();
}
void SExpressionParser::pop(void) {
	// TODO maybe just replace the entire scanner (making sure to copy input_value over).
	scanner->pop();
}
void SExpressionParser::parse_closing_brace(void) {
	// TODO auto)
	scanner->consume(Symbols::Srightparen);
}
int SExpressionParser::getPosition(void) const {
	return(scanner->getPosition());
}

}; /* end namespace Scanners */
