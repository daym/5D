/*
5D vector analysis program
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
#ifdef _WIN32
/* for fmemopen used in parse_simple... */
#include "stdafx.h"
#endif
namespace Scanners {
using namespace AST;

MathParser::MathParser(void) : Scanner() {
	B_process_macros = true;
	input_value = NULL;
	//operator_precedence_list = new OperatorPrecedenceList(false);
}
void MathParser::parse_structural(int input) {
	switch(input) {
	case '(':
		input_value = intern("(");
		return;
	case ')':
		input_value = intern(")");
		return;
	case '[':
		input_value = intern("(");
		return;
	case ']':
		input_value = intern(")");
		return;
	/* TODO other kind of braces? */
	default:
		raise_error("<expression>", input);	
	}
}
void MathParser::parse_string(int input) {
	std::stringstream matchtext;
	// assert input == '"'
	bool B_escaped = false;
	parse_optional_whitespace();
	for(++position, input = fgetc(input_file); input != EOF && (input != '"' || B_escaped); ++position, input = fgetc(input_file)) {
		if(input == '\n')
			++line_number;
		if(!B_escaped) {
			if(input == '\\') {
				B_escaped = true;
				continue;
			}
			matchtext << (char) input;
		} else { /* escaped */
			B_escaped = false;
			switch(input) {
			case '\\':
			default:
				matchtext << (char) input;
			}
		}
	}
	std::string value = matchtext.str();
	input_value = AST::str_literal(value.c_str());
}
static bool operatorCharP(int input) {
	// without '#' for now (not sure whether that's good. TODO find out)
	// without '@' for now (keywords).
	// without braces 40 41 91 93
	return((input >= 33 && input < 35) || (input >= 36 && input < 48 && input != '(' && input != ')') || (input >= 58 && input < 64) || (input == '^') || (input == '|') || (input == '~')) || input == '[' || input == ']';
}
void MathParser::parse_operator(int input) {
	std::stringstream sst;
	using namespace AST;
	// TODO UTF-8 math operators.
	if(!operatorCharP(input))
		return(parse_symbol(input));
	while(operatorCharP(input)) {
		sst << (char) input;
		++position, input = fgetc(input_file);
	}
	ungetc(input, input_file), --position;
	input_value = intern(sst.str().c_str());
}

void MathParser::parse_number_with_base(int input, int base) {
	int value = 0; /* TODO make this more general? */
	while(true) {
		int digit = (input >= '0' && input <= '9') ? (input - '0') :
		            (input >= 'A' && input <= 'Z') ? 10 + (input - 'A') :
		            (input >= 'a' && input <= 'z') ? 10 + (input - 'a') :
		            -1;
		if(digit == -1) {
			ungetc(input, input_file), --position;
			break;
		}
		if(digit < 0 || digit >= base)
			raise_error("<number>", input);
		value = value * base + digit;
		++position, input = fgetc(input_file);
	}
	std::stringstream sst;
	sst << value; /* decimal */
	input_value = AST::intern(sst.str().c_str());
}
void MathParser::parse_number(int input) {
	using namespace AST;
	std::stringstream matchtext;
	while((input >= '0' && input <= '9') || input == '.') {
		matchtext << ((char) input);
		++position, input = fgetc(input_file);
	}
	if(input == 'e' || input == 'E') {
		matchtext << ((char) input);
		++position, input = fgetc(input_file);
		while((input >= '0' && input <= '9') || input == '-' || input == '+'/* || input == '.'*/) {
			matchtext << ((char) input);                   
			++position, input = fgetc(input_file);
		}
	}
	ungetc(input, input_file), --position;
	input_value = AST::intern(matchtext.str().c_str());
	/* actual value will be provided by provide_dynamic_builtins */
}
void MathParser::parse_unicode(int input) {
	using namespace AST;
	if(input == 0xC2) {
		++position, input = fgetc(input_file);
		if(input == 0xAC) { // ¬
			input_value = intern("not");
		} else
			raise_error("¬", input);
		return;
	}
	if(input != 0xE2) {
		raise_error("<expression>", input);
		return;
	}
	++position, input = fgetc(input_file);
	if(input != 0x89) {
		if(input == 0x8B) {
			++position, input = fgetc(input_file);
			switch(input) {
			case 0x85: /* dot */
				input_value = intern("*");
				return;
			}
		} else if(input == 0xA8) {
			++position, input = fgetc(input_file);
			switch(input) {
			case 0xAF: /* ⨯ */
				input_value = intern("⨯");
				return;
			}
		} else { // E2 88 AB integral.
			parse_symbol(input, 0xE2);
			//raise_error("<expression>", input);
			return;
		}
	} else
		++position, input = fgetc(input_file);
	switch(input) {
	case 0xA0:
		input_value = intern("/=");
		return;
	case 0xA4:
		input_value = intern("≤");
		return;
	case 0xA5:
		input_value = intern("≥");
		return;
#if 0
	case 0x88: /* approx. */
		input_value = intern("≈");
		/* TODO just pass that to the symbol processor in the general case. */
		return;
#endif
	default:
		return(parse_symbol(input, 0xE2, 0x89));
		//raise_error("<operator>", input);
		return;
	}
}
void MathParser::parse_optional_whitespace(void) {
	int input;
	// skip whitespace...
	while(++position, input = fgetc(input_file), input == ' ' || input == '\t' || input == '\n' || input == '\r') {
		if(input == '\n')
			++line_number;
	}
	ungetc(input, input_file), --position;
}
void MathParser::parse_token(void) {
	parse_optional_whitespace();
	int input;
	++position, input = fgetc(input_file);
	switch(input) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		parse_number(input);
		break;
	case 0xE2: /* part of "≠" */
	case 0xC2:
		parse_unicode(input);
		break;
	case '\'':
		input_value = intern("'");
		break;
	case '\\':
		input_value = intern("\\");
		break;
	case '(':
	case ')':
	case '[':
	case ']':
		parse_structural(input);
		break;
	case '"':
		parse_string(input);
		break;
	case EOF:
		input_value = NULL;
		break;
	case '@':
		parse_keyword(input);
		break;
	case '#':
		parse_special_coding(input);
		break;
	default:
		parse_operator(input);
		break;
	}
}
static bool symbol1_char_P(int input) {
	return (input >= 'A' && input <= 'Z')
	    || (input >= 'a' && input <= 'z')
	    || input == '#'
	    // FIXME || input == '$'
	    || (input >= 128 && input != 0xE2 /* operators */);
}
bool symbol_char_P(int input) {
	return symbol1_char_P(input) 
	    || (input >= '0' && input <= '9') 
	    || /*input == '_' || */input == '?';
	  /*  || input == '^' not really part of the symbol name any more. */
}
void MathParser::parse_special_coding(int input) {
	assert(input == '#');
	++position, input = fgetc(input_file);
	switch(input) {
	case 'o':
	case 'x':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9': /* & "r" */
		{
			int base = input == 'o' ? 8 : input == 'x' ? 16 : 0;
			while(input >= '0' && input <= '9') {
				base = base * 10 + (input - '0');
				++position, input = fgetc(input_file);
			}
			/* TODO is #or1 valid? */
			if(input == 'r' || input == 'o' || input == 'x')
				++position, input = fgetc(input_file);
			parse_number_with_base(input, base);
		}
		break;
	//case '|': /* block comment */ TODO
	//	parse_block_comment();
	//	break;
	default:
		parse_symbol(input, '#');
		break;
	}
}
void MathParser::parse_symbol(int input, int special_prefix, int special_prefix_2) {
	std::stringstream matchtext;
	if(special_prefix) {
		matchtext << (char) special_prefix;
	}
	if(special_prefix_2) {
		matchtext << (char) special_prefix_2;
	}
	if(!symbol1_char_P(input)) {
		raise_error("<expression>", input);
		return;
	}
	while(symbol_char_P(input)) {
		matchtext << (char) input;
		++position, input = fgetc(input_file);
	}
	if(input == 0xE2) {
		++position, input = fgetc(input_file);
		if(input == 0x83) { // vector arrow etc.
			++position, input = fgetc(input_file);
			matchtext << (char) 0xE2 << (char) 0x83 << (char) input; // usually 0x97
		} else {
			ungetc(input, input_file), --position;
			ungetc(0xE2, input_file), --position; // FIXME it is actually unsupported to unget more than 1 character :-(
			//raise_error("<unicode_operator>", "<unknown>");
		}
	} else
		ungetc(input, input_file), --position;
	input_value = intern(matchtext.str().c_str());
}
void MathParser::parse_keyword(int input) {
	std::stringstream matchtext;
	++position, input = fgetc(input_file);
	if(!symbol1_char_P(input)) {
		raise_error("<symbol>", input);
		return;
	}
	while(symbol_char_P(input)) {
		matchtext << (char) input;
		++position, input = fgetc(input_file);
	}
	if(input != ':') {
		raise_error(":", input);
		return;
	}
	matchtext << (char) input;
	//++position, input = fgetc(input_file);
	//ungetc(input, input_file), --position;
	input_value = keywordFromString(matchtext.str().c_str());
}
/* returns the PREVIOUS value */
AST::Node* MathParser::consume(AST::Symbol* expected_value) {
	AST::Node* previous_value;
	previous_value = input_value;
	if(expected_value && expected_value != input_value)
		raise_error(expected_value->name, input_value ? input_value->str() : "<nothing>");
	previous_position = position;
	parse_token();
	return(previous_value);
}
using namespace AST;
AST::Cons* MathParser::operation(AST::Node* operator_, AST::Node* operand_1, AST::Node* operand_2) {
	if(operator_ == NULL || operand_1 == NULL/* || operand_2 == NULL*/) {
		raise_error("<second_operand>", "<nothing>");
		return(NULL);
	} else if(operator_ == intern(" ")) // apply
		return(cons(operand_1, cons(operand_2, NULL)));
	else
		return(cons(cons(operator_, cons(operand_1, NULL)), cons(operand_2, NULL)));
		//return(cons(operator_, cons(operand_1, cons(operand_2, NULL))));
}
bool macro_operator_P(AST::Node* operator_) {
	return(operator_ == intern("define") || operator_ == intern("'") || operator_ == intern("["));
}
AST::Node* MathParser::maybe_parse_macro(AST::Node* node) {
	if(B_process_macros && macro_operator_P(node))
		return(parse_macro(node));
	else
		return(NULL);
}
AST::Node* MathParser::parse_define(AST::Node* operand_1) {
	bool B_extended = (input_value == AST::intern("("));
	if(B_extended)
		consume();
	AST::Node* parameter = consume();
	if(dynamic_cast<AST::Symbol*>(parameter) == NULL) {
		raise_error("<symbol>", parameter->str());
	}
	if(B_extended)
		consume(AST::intern(")"));
	//AST::Node* parameter = (input_token == intern("<symbol>")) ? consume(intern("<symbol>")) : consume(intern("<operator>"));
	AST::Node* body = parse_expression();
	if(!parameter||!body||!operand_1) {
		raise_error("<define-body>", "<incomplete>");
		return(NULL);
	}
	return(cons(operand_1, cons(parameter, cons(body, NULL))));
	//return(cons(operand_1, cons(parse_expression(), NULL)));
}
AST::Node* MathParser::parse_quote(AST::Node* operand_1) {
	AST::Node* result;
	B_process_macros = false; /* to make (' define) work; TODO do this in a nicer way? How? */
	result = cons(operand_1, cons(parse_expression(), NULL));
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
	else if(operand_1 == intern("'"))
		return(parse_quote(operand_1));
	else if(operand_1 == intern("["))
		return(parse_list());
	else {
		raise_error("<known_macro>", "<unknown_macro>");
		return(NULL);
	}
}
AST::Cons* MathParser::unary_operation(AST::Node* operator_, AST::Node* operand_1) {
	assert(operator_);
	return(cons(operator_, cons(operand_1, NULL)));
}
AST::Node* MathParser::parse_abstraction(void) {
	if(dynamic_cast<AST::Symbol*>(input_value) == NULL) {
		raise_error("<symbol>", input_value ? input_value->str() : "nothing");
		return(NULL);
	} else {
		AST::Node* parameter = consume();
		AST::Node* expression = parse_expression();
		if(expression)
			return(cons(intern("\\"), cons(parameter, cons(expression, NULL))));
		else // ???
			return(cons(intern("\\"), cons(parameter, NULL)));
	}
}
AST::Node* MathParser::parse_value(void) {
	if(input_value == intern(")")) {
		raise_error("<value>", ')');
		return(NULL);
	} else if(input_value == intern("\\")) { // function abstraction
		consume();
		return(parse_abstraction());
#if 0
	} else if(input_value == intern("-") || input_value == intern("+") || input_value == intern("~") || input_value == intern("'")) {
		/* the reason why there is a special-case for "~" at all is so that it will require an argument.
		   Otherwise stuff like ~~#t would not work as expected. 
		   If "~" were a normal functional, there would be no reason to expect this to be a call.
		   Hence, ~~#t would mean "(~ ~) whatsthatjunk" or in the best case "(~ ~) #t", both of which is NOT how it is usually meant.
		   It is usually meant ~ (~ #t). Hence the following special-case forcing it to read the argument if there is one after it.
		   The same can be said for all unary operators, so there should be special cases for all of them.
		   Best would be to get rid of all unary operators. TODO.
		 */
		AST::Node* operator_ = consume();
		if(input_value == AST::intern(")") || input_value == NULL) {
			return(operator_);
		}
		AST::Node* argument = parse_binary_operation(operator_precedence_list->next_precedence_level(-1)); // FIXME
		//input_value == intern("~") ? negation_precedence_level : minus_precedence_level - 1);
		//AST::Node* argument = parse_value();
		if(argument == NULL)
			return(operator_);
		else
			return((operator_ == intern("+")) ? argument :
			       (operator_ == intern("-")) ? operation(intern("-"), intern("0"), argument) :
			       unary_operation(operator_, argument));
#endif
		/*} else if(input_token == intern("<string>")) {
		return(consume());*/
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
				raise_error(")", input_value ? input_value->str() : "<nothing>");
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
		{
			AST::Node* macro_result;
			macro_result = maybe_parse_macro(result);
			if(macro_result)
				return(macro_result);
		}
		return(result);
	}
}
AST::Node* MathParser::parse_binary_operation(int precedence_level) {
	struct AST::Symbol* associativity;
	bool B_visible_operator, B_unary_operator = false;
	//printf("level is %d, input is: %s\n", precedence_level, input_value->str().c_str());
	if(operator_precedence_list->empty_P(precedence_level))
		return(parse_value());
	/* special case for unary - */
	AST::Node* result = (precedence_level == MINUS_PRECEDENCE_LEVEL && input_value == intern("-")) ? (B_unary_operator = true, intern("0")) : parse_binary_operation(operator_precedence_list->next_precedence_level(precedence_level));
	if(AST::Node* actual_token = operator_precedence_list->match_operator(precedence_level, input_value, /*out*/associativity, /*out*/B_visible_operator)) {
		while(actual_token) {
			AST::Node* operator_ = B_visible_operator ? consume() : intern(" ");
			if(input_value == intern(")")) // premature end.
				return(B_unary_operator ? operator_ : cons(operator_, cons(result, NULL))); /* default to the binary operator */
			AST::Node* b = parse_binary_operation(associativity != intern("right") ? operator_precedence_list->next_precedence_level(precedence_level) : precedence_level);
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
		return parse_binary_operation(operator_precedence_list->next_precedence_level(-1));
	else
		return parse_value();
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
		return(cons(head, parse_S_list_body()));
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
		parser.push(input_file, 0);
		result = parser.parse(operator_precedence_list);
		fclose(input_file);
		return(result);
	} catch(ParseException& exception) {
		fprintf(stderr, "could not parse \"%s\" because: %s\n", text, exception.what());
		abort();
	}
}
void MathParser::push(FILE* input_file, int line_number, bool B_consume) {
	Scanner::push(input_file, line_number, B_consume);
	if(B_consume)
		consume();
}
void MathParser::parse_closing_brace(void) {
	consume(AST::intern(")"));
}
bool MathParser::EOFP(void) const {
	return(input_value == NULL);
}
void MathParser::ensure_end(void) {
	if(!EOFP())
		raise_error("<EOF>", input_value->str());
}

};
