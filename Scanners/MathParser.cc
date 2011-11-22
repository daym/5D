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
#include <string.h>
#include "Scanners/MathParser"
#include "Scanners/OperatorPrecedenceList"
#include "AST/Symbol"
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

MathParser::MathParser(void) : Scanner() {
	B_process_macros = true;
	input_value = NULL;
	//operator_precedence_list = new OperatorPrecedenceList(false);
}
using namespace AST;
AST::Node* MathParser::operation(AST::Node* operator_, AST::Node* operand_1, AST::Node* operand_2) {
	AST::Node* result = makeOperation(operator_, operand_1, operand_2);
	if(result == NULL)
		raise_error("<second_operand>", "<nothing>");
	return(result);
}
bool macro_operator_P(AST::Node* operator_) {
	return(operator_ == intern("define") || operator_ == intern("defrec") || operator_ == intern("'") || operator_ == intern("["));
}
/*
AST::Node* MathParser::maybe_parse_macro(AST::Node* node) {
	if(B_process_macros && macro_operator_P(node))
		return(parse_macro(node));
	else
		return(NULL);
}
*/
AST::Node* MathParser::parse_define(AST::Node* operand_1) {
	bool B_extended = (input_value == AST::intern("("));
	if(B_extended)
		consume();
	AST::Node* parameter = consume();
	if(dynamic_cast<AST::Symbol*>(parameter) == NULL) {
		raise_error("<symbol>", str(parameter));
	}
	if(B_extended)
		consume(AST::intern(")"));
	//AST::Node* parameter = (input_token == intern("<symbol>")) ? consume(intern("<symbol>")) : consume(intern("<operator>"));
	AST::Node* body = parse_expression();
	if(!parameter||!body||!operand_1) {
		raise_error("<define-body>", "<incomplete>");
		return(NULL);
	}
	return(makeApplication(operand_1, makeAbstraction(parameter, body)));
}
AST::Node* MathParser::parse_defrec(AST::Node* operand_1) {
	bool B_extended = (input_value == AST::intern("("));
	if(B_extended)
		consume();
	AST::Node* parameter = consume();
	if(dynamic_cast<AST::Symbol*>(parameter) == NULL) {
		raise_error("<symbol>", str(parameter));
	}
	if(B_extended)
		consume(AST::intern(")"));
	//AST::Node* parameter = (input_token == intern("<symbol>")) ? consume(intern("<symbol>")) : consume(intern("<operator>"));
	AST::Node* body = parse_expression();
	if(!parameter||!body||!operand_1) {
		raise_error("<define-body>", "<incomplete>");
		return(NULL);
	}
	return(makeApplication(AST::intern("define"), makeAbstraction(parameter, makeApplication(AST::intern("rec"), makeAbstraction(parameter, body)))));
}
AST::Node* MathParser::parse_quote(AST::Node* operand_1) {
	AST::Node* result;
	B_process_macros = false; /* to make (' define) work; TODO do this in a nicer way? How? */
	result = makeApplication(operand_1, parse_value());
	B_process_macros = true;
	return(result);
}
AST::Node* MathParser::parse_list(void) {
	if(EOFP() || input_value == intern("]")) {
		consume(intern("]"));
		return(NULL);
	} else {
		AST::Node* value = parse_value();
		return(operation(intern(":"), value, parse_list()));
	}
}
AST::Node* MathParser::parse_macro(AST::Node* operand_1) {
	// TODO let|where, include, cond, make-list, quote, case.
	if(operand_1 == intern("define"))
		return(parse_define(operand_1));
	if(operand_1 == intern("defrec"))
		return(parse_defrec(operand_1));
	else if(operand_1 == intern("'"))
		return(parse_quote(operand_1));
	else if(operand_1 == intern("["))
		return(parse_list());
	else {
		raise_error("<known_macro>", "<unknown_macro>");
		return(NULL);
	}
}
AST::Node* MathParser::parse_application(void) {
	AST::Node* hd = parse_value();
#ifndef SIMPLE_APPLICATION
	while(!EOFP() && input_value != AST::intern(")") && input_value != AST::intern("]") && input_value && !operator_precedence_list->any_operator_P(input_value)) {
		hd = AST::makeApplication(hd, parse_argument());
	}
#endif
	return(hd);
}
AST::Node* MathParser::parse_abstraction(void) {
	if(dynamic_cast<AST::Symbol*>(input_value) == NULL) {
		raise_error("<symbol>", str(input_value));
		return(NULL);
	} else {
		AST::Node* parameter = consume();
		if(EOFP() || input_value == AST::intern(")") || input_value == AST::intern("]"))
			raise_error("<body>", str(input_value));
		AST::Node* expression = parse_expression();
		if(expression)
			return(makeAbstraction(parameter, expression));
		else // ???
			return(makeAbstraction(parameter, NULL));
	}
}
AST::Node* MathParser::parse_value(void) {
	if(input_value == intern(")")) {
		raise_error("<value>", ')');
		return(NULL);
	} else if(input_value == intern("\\")) { // function abstraction
		consume();
		return(parse_abstraction());
	} else {
		AST::Node* result;
		if(input_value == intern("(")) {
			AST::Node* opening_brace = input_value;
			consume();
			if(input_value == intern(")"))
				result = NULL;
			else
				result = parse_expression();
			if(opening_brace != intern("(") || input_value != intern(")")) {
				raise_error(")", str(input_value));
				return(NULL);
			}
			consume(intern(")"));
		} else {
#if 0
			if(input_value == AST::intern(")")) { /* oops! */
				raise_error("<value>", ")");
			} else if(input_value == NULL) {
				raise_error("<value>", "<EOF>");
			}
#endif
			result = consume();
		}
		if(B_process_macros && macro_operator_P(result))
			return(parse_macro(result));
		else
			return(result);
	}
}
AST::Node* MathParser::parse_binary_operation(bool B_allow_args, int precedence_level) {
	struct AST::Symbol* associativity;
	bool B_visible_operator, B_unary_operator = false;
	//printf("level is %d, input is: %s\n", precedence_level, input_value->str().c_str());
	if(operator_precedence_list->empty_P(precedence_level))
		return(B_allow_args ? parse_application() : parse_value());
	/* special case for unary - */
	AST::Node* result = (precedence_level == MINUS_PRECEDENCE_LEVEL && input_value == intern("-")) ? (B_unary_operator = true, intern("0")) : parse_binary_operation(B_allow_args, operator_precedence_list->next_precedence_level(precedence_level));
	if(AST::Node* actual_token = operator_precedence_list->match_operator(precedence_level, input_value, /*out*/associativity, /*out*/B_visible_operator)) {
		while(actual_token) {
			AST::Node* operator_ = B_visible_operator ? consume() : intern(" ");
			if(input_value == intern(")")) // premature end.
				return(B_unary_operator ? operator_ : makeApplication(operator_, result)); /* default to the binary operator */
			AST::Node* b = parse_binary_operation(B_allow_args, associativity != intern("right") ? operator_precedence_list->next_precedence_level(precedence_level) : precedence_level);
			if(B_unary_operator && !b) // -nil
				return(operator_);
			result = operation(operator_, result, b);
			/* for right associative operations, the recursion will have consumed all the operators on that level and by virtue of that, the while loop will always stop after one iteration. */
			if(associativity == intern("none"))
				break;
			actual_token = operator_precedence_list->match_operator(precedence_level, input_value, /*out*/associativity, /*out*/B_visible_operator);
		}
		return(result);
	} else
		return(result);
}
AST::Node* MathParser::parse_expression(void) {
	if(operator_precedence_list)
		return parse_binary_operation(true, operator_precedence_list->next_precedence_level(-1));
	else
		return parse_application();
}
AST::Node* MathParser::parse_argument(void) {
	assert(operator_precedence_list->apply_level != 0);
	return parse_binary_operation(false, operator_precedence_list->apply_level);
}
AST::Node* MathParser::parse(OperatorPrecedenceList* operator_precedence_list) {
	this->operator_precedence_list = operator_precedence_list;
	AST::Node* result = parse_expression();
	return(result);
}
AST::Cons* MathParser::parse_S_list_body(void) {
	if(input_value == intern(")"))
		return(NULL);
	else {
		AST::Node* head;
		head = parse_S_Expression();
		return(makeCons(head, parse_S_list_body()));
	}
}
AST::Cons* MathParser::parse_S_list(bool B_consume_closing_brace) {
	AST::Cons* result = NULL;
	consume(intern("("));
	/* TODO macros (if we want) */
	result = parse_S_list_body();
	if(B_consume_closing_brace)
		consume(intern(")"));
	return(result);
}
AST::Node* MathParser::parse_S_Expression(void) {
	/* TODO do this without tokenizing? How? */
	if(input_value == intern("(")) {
		return(parse_S_list(true));
	} else if(dynamic_cast<AST::Symbol*>(input_value) != NULL) {
		return(consume()); // & whitespace.
	} else {
		/* numbers, strings */
		if(input_value)
			return(consume());
		else {
			raise_error("<S_Expression>", "<junk>");
			return(NULL);
		}
		//parse_S_optional_whitespace();
	}
}
AST::Node* MathParser::parse_simple(const char* text, OperatorPrecedenceList* operator_precedence_list) {
	AST::Node* result;
	MathParser parser;
	FILE* input_file;
	try {
		input_file = fmemopen((void*) text, strlen(text), "r");
		parser.push(input_file, 0, false);
		parser.consume();
		result = parser.parse(operator_precedence_list);
		fclose(input_file);
		return(result);
	} catch(ParseException& exception) {
		fprintf(stderr, "could not parse \"%s\" because: %s\n", text, exception.what());
		abort();
	}
}
void MathParser::parse_closing_brace(void) {
	consume(AST::intern(")"));
}

};
