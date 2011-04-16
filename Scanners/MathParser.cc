/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <assert.h>
#include <iostream>
#include <string.h>
#include "Scanners/MathParser"
#include "AST/Symbol"
#include "AST/AST"

namespace Scanners {
using namespace AST;

void MathParser::parse_structural(int input) {
	switch(input) {
	case '(':
		input_value = input_token = intern("(");
		return;
	case ')':
		input_value = input_token = intern(")");
		return;
	/* TODO other kind of braces? */
	default:
		raise_error("<expression>", input);	
	}
}

void MathParser::parse_operator(int input) {
	using namespace AST;
	switch(input) {
	case '.':
		input_value = input_token = intern(".");
		break;
	case '^':
		input_value = input_token = intern("^");
		break;
	case '+':
		input_value = input_token = intern("+");
		break;
	case '-':
		input_value = input_token = intern("-");
		break;
	case '*':
		++position, input = fgetc(input_file);
		if(input == '*') {
			// FIXME exponentiation is right-associative.
			input_value = input_token = intern("**");
		} else {
			ungetc(input, input_file);
			input_value = input_token = intern("*");
		}
		break;
	case '/':
		++position, input = fgetc(input_file);
		if(input == '=') { /* not equal */
			input_value = input_token = intern("/=");
		} else {
			ungetc(input, input_file);
			input_value = input_token = intern("/");
		}
		break;
	case '%':
		input_value = input_token = intern("/");
		break;
	case '=':
		input_value = input_token = intern("=");
		break;
	case '<':
		++position, input = fgetc(input_file);
		if(input == '=')
			input_value = input_token = intern("<=");
		else {
			ungetc(input, input_file);
			input_value = input_token = intern("<");
		}
		break;
	case '>':
		++position, input = fgetc(input_file);
		if(input == '=')
			input_value = input_token = intern(">=");
		else {
			ungetc(input, input_file);
			input_value = input_token = intern(">");
		}
		break;
	case '&':
		input_value = input_token = intern("&");
		break;
	case '|':
		input_value = input_token = intern("|");
		break;
	default:
		raise_error("<operator>", input);
	}
}

void MathParser::parse_star(int input) {
	parse_operator('*');
}
void MathParser::parse_anglebracket(int input) {
	parse_operator(input);
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
	ungetc(input, input_file);
	input_token = intern("<number>");
	input_value = literal(strdup(matchtext.str().c_str()));
}
void MathParser::parse_unicode(int input) {
	using namespace AST;
	if(input == 0xC2) {
		++position, input = fgetc(input_file);
		if(input == 0xAC) { // ¬
			input_token = intern("~");
			input_value = intern("¬");
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
				input_value = input_token = intern("*");
				return;
			}
		} else if(input == 0xA8) {
			++position, input = fgetc(input_file);
			switch(input) {
			case 0xAF: /* ⨯ */
				input_value = input_token = intern("⨯");
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
		input_token = intern("/=");
		input_value = input_token;
		return;
	case 0xA4:
		input_token = intern("<=");
		input_value = intern("≤");
		return;
	case 0xA5:
		input_token = intern(">=");
		input_value = intern("≥");
		return;
	default:
		raise_error("<operator>", input);
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
	ungetc(input, input_file);
}
void MathParser::parse_S_optional_whitespace(void) {
	int input;
	// skip whitespace...
	while(++position, input = fgetc(input_file), input == ' ' || input == '\t' || input == '\n' || input == '\r') {
		if(input == '\n')
			++line_number;
	}
	ungetc(input, input_file);
}
void MathParser::parse_token(void) {
	int input;
	++position, input = fgetc(input_file);
	switch(input) {
	case 0xE2: /* part of "≠" */
	case 0xC2:
		parse_unicode(input);
		break;
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
	case '*':
		parse_star(input);
		break;
	case '<':
	case '>':
		parse_anglebracket(input);
		break;
	case '~':
		input_value = input_token = intern("~");
		break;
	case '+':
	case '-':
	case '/':
	case '%':
	case '=':
	case '&':
	case '|':
	case '.':
	case '^':
		parse_operator(input);
		break;
	case '\\':
		input_value = input_token = intern("\\");
		break;
	case '(':
	case ')':
		parse_structural(input);
		break;
	case EOF:
		input_value = input_token = NULL;
		break;
	default:
		parse_symbol(input);
		break;
	}
	parse_optional_whitespace();
}
static bool symbol1_char_P(int input) {
	return (input >= '@' && input <= 'Z')
	    || (input >= 'a' && input <= 'z')
	    || (input >= 128 && input != 0xE2 /* operators */);
}
static bool symbol_char_P(int input) {
	return symbol1_char_P(input) 
	    || (input >= '0' && input <= '9') 
	    || input == '_';
	  /*  || input == '^' not really part of the symbol name any more. */
}
void MathParser::parse_symbol(int input, int special_prefix) {
	std::stringstream matchtext;
	if(special_prefix) {
		matchtext << (char) special_prefix;
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
			ungetc(input, input_file);
			ungetc(0xE2, input_file); // FIXME it is actually unsupported to unget more than 1 character :-(
			//raise_error("<unicode_operator>", "<unknown>");
		}
	} else
		ungetc(input, input_file);
	input_token = intern("<symbol>");
	input_value = intern(matchtext.str().c_str());
}
/* returns the PREVIOUS value */
AST::Node* MathParser::consume(AST::Symbol* expected_token) {
	AST::Node* previous_value;
	previous_value = input_value;
	if(expected_token && expected_token != input_token)
		raise_error(expected_token->name, input_token ? input_token->name : "<nothing>");
	parse_token();
	return(previous_value);
}
using namespace AST;
/* keep apply_precedence_level in sync with operator_precedence below */
int apply_precedence_level = 3;
static Symbol* operator_precedence[][7] = {
	{intern("."), intern("^")},
	{intern("**")},
	{intern("*"), intern("%"), intern("/")},
	{intern("⨯")},
	{intern("+"), intern("-")},
	{intern("="), intern("/=")},
	{intern("<"), intern("<="), intern(">"), intern(">=") /*, intern("≤"), intern("≥")*/},
	{intern("&")},
	//{intern("^")}
	{intern("|")},
};
int get_operator_precedence(AST::Symbol* symbol) {
	if(symbol == NULL)
		return(-1);
	for(size_t i = 0; i < sizeof(operator_precedence) / sizeof(operator_precedence[0]); ++i)
		for(size_t j = 0; operator_precedence[i][j]; ++j)
			if(operator_precedence[i][j] == symbol)
				return(i);
	return(-1);
}
static bool any_operator_P(AST::Node* input_token, int first_precedence_level, int frontier_precedence_level);
static AST::Node* match_operator(int precedence_level, AST::Node* input_token) {
	for(int i = 0; operator_precedence[precedence_level][i]; ++i)
		if(operator_precedence[precedence_level][i] == input_token)
			return(input_token);
	return(NULL);
}
static bool any_operator_P(AST::Node* input_token, int first_precedence_level, int frontier_precedence_level) {
	for(int i = first_precedence_level; i < frontier_precedence_level; ++i)
		if(match_operator(i, input_token))
			return(true);
	return(false);
}
AST::Cons* MathParser::operation(AST::Node* operator_, AST::Node* operand_1, AST::Node* operand_2) {
	if(operator_ == NULL || operand_1 == NULL || operand_2 == NULL) {
		raise_error("<second_operand>", "<nothing>");
		return(NULL);
	} else if(operator_ == intern("apply"))
		return(cons(operand_1, cons(operand_2, NULL)));
	else
		return(cons(operator_, cons(operand_1, cons(operand_2, NULL))));
}
static bool macro_operator_P(AST::Node* operator_) {
	return(operator_ == intern("define"));
}
AST::Node* MathParser::maybe_parse_macro(AST::Node* node) {
	if(macro_operator_P(node))
		return(parse_macro(node));
	else
		return(NULL);
}
AST::Node* MathParser::parse_macro(AST::Node* operand_1) {
	if(operand_1 == intern("define")) {
		AST::Node* parameter = consume(intern("<symbol>"));
		AST::Node* body = parse_expression();
		if(!parameter||!body||!operand_1) {
			raise_error("<define-body>", "<incomplete>");
			return(NULL);
		}
		return(cons(operand_1, cons(parameter, cons(body, NULL))));
		//return(cons(operand_1, cons(parse_expression(), NULL)));
	} else {
		raise_error("<known_macro>", "<unknown_macro>");
		return(NULL);
	}
}
static AST::Cons* operation(AST::Node* operator_, AST::Node* operand_1) {
	assert(operator_);
	return(cons(operator_, cons(operand_1, NULL)));
}
AST::Node* MathParser::parse_abstraction(void) {
	if(input_token != intern("<symbol>")) {
		raise_error("<symbol>", input_token ? input_token->str() : "nothing");
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
	if(input_token == intern("\\")) { // function abstraction
		consume();
		return(parse_abstraction());
	} else if(input_token == intern("-") || input_token == intern("+") || input_token == intern("~")) {
		AST::Node* operator_ = consume();
		AST::Node* argument = parse_value();
		if(argument == NULL)
			raise_error("<operand>", "<nothing>");
		return((operator_ == intern("+")) ? argument :
		       (operator_ == intern("-")) ? cons(intern("0-"), cons(argument, NULL)) :
		       cons(operator_, cons(argument, NULL)));
	} else {
		AST::Node* result;
		if(input_token == intern("(")) {
			bool previous_allow_args = allow_args;
			allow_args = true;
			AST::Node* opening_brace = input_value;
			consume();
			result = parse_expression();
			if(opening_brace != intern("(") || input_value != intern(")")) {
				raise_error(")", input_value ? input_value->str() : "<nothing>");
				allow_args = previous_allow_args;
				return(NULL);
			}
			consume(intern(")"));
			allow_args = previous_allow_args;
		} else
			result = consume();
		{
			AST::Node* macro_result;
			macro_result = maybe_parse_macro(result);
			if(macro_result)
				return(macro_result);
		}
		if(allow_args) {
			allow_args = false; // sigh...
			try {
				// this will have problems with: "cos -3" because it doesn't know that that means "cos (-3)" and not "(cos)-3".
				while(!(input_token == NULL || input_token == intern(")") || any_operator_P(input_token, 0, sizeof(operator_precedence)/sizeof(operator_precedence[0])))) {
					//raise_error("<operand>", result ? result->str() : "<nothing>");
					result = operation(intern("apply"), result, parse_argument());
				}
			} catch(...) {
				allow_args = true;
				throw;
			}
			allow_args = true;
		}
		return(result);
	}
	// TODO []
	// TODO .
	// TODO ~ (not)
}
AST::Node* MathParser::parse_binary_operation(int precedence_level) {
	if(precedence_level < 0)
		return(parse_value());
	AST::Node* result = parse_binary_operation(precedence_level - 1);
	// FIXME
	//if(result == intern(")") || any_operator_P(result, apply_precedence_level + 1, sizeof(operator_precedence)/sizeof(operator_precedence[0]))) {
	//	raise_error("<operand>", result ? result->str() : "<nothing>");
	//}
	while(AST::Node* actual_token = match_operator(precedence_level, input_token)) {
		AST::Node* operator_ = actual_token;
		consume(); /* operator */
		result = operation(operator_, result, parse_binary_operation(precedence_level - 1));
	}
	return(result);
}
AST::Node* MathParser::parse_expression(void) {
	return parse_binary_operation(sizeof(operator_precedence)/sizeof(operator_precedence[0]) - 1);
}
AST::Node* MathParser::parse_argument(void) {
	return parse_binary_operation(apply_precedence_level);
}
AST::Node* MathParser::parse(FILE* input_file) {
	push(input_file, 0);
	allow_args = true;
	consume();
	AST::Node* result = parse_expression();
	pop();
	return(result);
}

AST::Cons* MathParser::parse_S_list(void) {
	if(input_token == intern(")"))
		return(NULL);
	else {
		AST::Node* head;
		head = parse_S_Expression_inline();
		return(cons(head, parse_S_list()));
	}
}

AST::Node* MathParser::parse_S_Expression_inline(void) {
	/* TODO do this without tokenizing? How? */
	if(input_token == intern("(")) {
		AST::Cons* result = NULL;
		consume();
		/* TODO macros (if we want) */
		result = parse_S_list();
		consume(intern(")"));
		parse_S_optional_whitespace();
		return(result);
	} else if(input_token == intern("<symbol>")) {
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

AST::Node* MathParser::parse_S_Expression(FILE* input_file) {
	push(input_file, 0);
	allow_args = true;
	consume();
	AST::Node* result = parse_S_Expression_inline();
	pop();
	return(result);
}

};
