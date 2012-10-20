#ifdef OLD_PARSER
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
#include "AST/Symbols"
#include "Values/Values"
#include "AST/Keyword"
#include "Evaluators/Builtins"
#ifdef _WIN32
/* for fmemopen used in parse_simple... */
#include "stdafx.h"
#endif
// If you plan to revive this, handle (and ignore) operator associativity Sprefix. remove parse_abstraction and instead just use unary operator "macro" for \\.
// handle all the macros as unary operators
namespace Scanners {
using namespace AST;
using namespace Evaluators;

MathParser::MathParser(void) : Scanner() {
	B_process_macros = true;
	input_value = NULL;
	bound_symbols = NULL;
	B_honor_indentation = true;
	//operator_precedence_list = new OperatorPrecedenceList(false);
}
using namespace AST;
AST::NodeT MathParser::operation(AST::NodeT operator_, AST::NodeT operand_1, AST::NodeT operand_2) {
	AST::NodeT result = makeOperation(operator_, operand_1, operand_2);
	//if(result == NULL)
	//	raise_error("<second_operand>", "<nothing>");
	return(result);
}
bool macro_operator_P(AST::NodeT operator_) {
	return(operator_ == Symbols::Sdefine || operator_ == Symbols::Sdef || operator_ == Symbols::Sdefrec || operator_ == Symbols::Squote || operator_ == Symbols::Sleftbracket || operator_ == Symbols::Slet);
}
/*
AST::NodeT MathParser::maybe_parse_macro(AST::NodeT node) {
	if(B_process_macros && macro_operator_P(node))
		return(parse_macro(node));
	else
		return(NULL);
}
*/
static AST::NodeT makeDefine(AST::NodeT parameter, AST::NodeT body) {
	AST::NodeT result = makeApplication(makeApplication(Symbols::Sdefine, parameter), body);
	return(result);
}
AST::NodeT MathParser::parse_define(AST::NodeT operand_1) {
	bool B_extended = (input_value == Symbols::Sleftparen);
	if(B_extended)
		consume();
	if(dynamic_cast<AST::Symbol*>(input_value) == NULL) {
		raise_error("<symbol>", str(input_value));
		return(NULL);
	}
	AST::NodeT parameter = consume();
	if(B_extended)
		consume(Symbols::Srightparen);
	//AST::NodeT parameter = (input_token == intern("<symbol>")) ? consume(intern("<symbol>")) : consume(intern("<operator>"));
	AST::NodeT body = parse_expression();
	if(!parameter||!body||!operand_1) {
		raise_error("<define-body>", "<incomplete>");
		return(NULL);
	}
	if(symbol_P(parameter))
		parameter = quote(parameter);
	if(dynamic_cast<AST::Abstraction*>(body))
		body = quote(body);
	return(makeDefine(parameter, body));
}
AST::NodeT MathParser::parse_defrec(AST::NodeT operand_1) {
	bool B_extended = (input_value == Symbols::Sleftparen);
	if(B_extended)
		consume();
	AST::NodeT parameter = consume();
	AST::Symbol* rparameter = dynamic_cast<AST::Symbol*>(parameter);
	if(dynamic_cast<AST::Symbol*>(parameter) == NULL) {
		raise_error("<symbol>", str(parameter));
	}
	if(B_extended)
		consume(Symbols::Srightparen);
	//AST::NodeT parameter = (input_token == intern("<symbol>")) ? consume(intern("<symbol>")) : consume(intern("<operator>"));
	if(dynamic_cast<AST::Symbol*>(parameter))
		parameter = quote(parameter);
	//if(dynamic_cast<AST::Abstraction*>(body))
	//	body = quote(body);
	enter_abstraction(rparameter);
	try {
		AST::NodeT body = parse_expression();
		if(!parameter||!body||!operand_1) {
			raise_error("<define-body>", "<incomplete>");
			return(NULL); // not reached
		}
		leave_abstraction(rparameter);
		return(makeDefine(quote(parameter), quote(makeApplication(Symbols::Srec, makeAbstraction(rparameter, body)))));
	} catch(...) {
		leave_abstraction(rparameter);
		throw;
	}
}
AST::NodeT MathParser::parse_quote(AST::NodeT operand_1) {
	AST::NodeT result;
	B_process_macros = false; /* to make it possible to (quote quote) and (quote define); TODO make it possible to use other macros anyway? */
	result = makeApplication(operand_1, parse_value());
	B_process_macros = true;
	return(result);
}
AST::NodeT MathParser::parse_let_form(void) {
	if(EOFP()) {
		raise_error("<let-form-body>", "<incomplete>");
		return(NULL);
	}
	AST::Symbol* name = dynamic_cast<AST::Symbol*>(consume());
	if(name == NULL) {
		raise_error("<let-form-symbol>", "<incomplete>");
		return(NULL);
	}
	consume(Symbols::Sequal);
	AST::NodeT value = parse_value();
	consume(Symbols::Sin);
	enter_abstraction(name);
	try {
		AST::NodeT rest = parse_expression();
		leave_abstraction(name);
		return(AST::makeApplication(AST::makeAbstraction(name, rest), value));
	} catch(...) {
		leave_abstraction(name);
		throw;
	}
}
AST::NodeT MathParser::parse_list(void) {
	if(EOFP() || input_value == Symbols::Srightbracket) {
		consume(Symbols::Srightbracket);
		return(NULL);
	} else {
		AST::NodeT value = parse_value();
		return(operation(Symbols::Scolon, value, parse_list()));
	}
}
AST::NodeT MathParser::parse_macro(AST::NodeT operand_1) {
	// TODO let|where, include, cond, make-list, quote, case.
	if(operand_1 == Symbols::Sdefine)
		return(parse_define(operand_1));
	else if(operand_1 == Symbols::Sdef)
		return(parse_define(operand_1));
	else if(operand_1 == Symbols::Sdefrec)
		return(parse_defrec(operand_1));
	else if(operand_1 == Symbols::Squote)
		return(parse_quote(operand_1));
	else if(operand_1 == Symbols::Sleftbracket)
		return(parse_list());
	else if(operand_1 == Symbols::Slet)
		return(parse_let_form());
	else {
		raise_error("<known_macro>", "<unknown_macro>");
		return(NULL);
	}
}
AST::NodeT MathParser::parse_application(void) {
	AST::NodeT hd = parse_value();
	return(hd);
}
AST::NodeT MathParser::parse_abstraction(void) {
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
			AST::NodeT expression = parse_expression();
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
AST::NodeT MathParser::parse_value(void) {
	if(input_value == Symbols::Srightparen || input_value == Symbols::Sautorightparen) {
		raise_error("<value>", ')');
		return(NULL);
	} else if(input_value == Symbols::SlessEOFgreater) {
		raise_error("<value>", "<EOF>");
		return(NULL);
	} else if(input_value == Symbols::Sbackslash) { // function abstraction
		consume();
		return(parse_abstraction());
	} else {
		AST::NodeT result;
		if(input_value == Symbols::Sleftparen || input_value == Symbols::Sautoleftparen) {
			bool prev_B_honor_indentation = B_honor_indentation;
			//if(input_value == Symbols::Sleftparen)
			//	B_honor_indentation = false;
			try {
				AST::NodeT opening_brace = consume();
				if((opening_brace == Symbols::Sleftparen && input_value == Symbols::Srightparen) ||
				   (opening_brace == Symbols::Sautoleftparen && input_value == Symbols::Sautorightparen))
					result = NULL;
				else
					result = parse_expression();
				if((opening_brace == Symbols::Sleftparen && input_value == Symbols::Srightparen) ||
				   (opening_brace == Symbols::Sautoleftparen && input_value == Symbols::Sautorightparen)) {
					consume();
					B_honor_indentation = prev_B_honor_indentation; // TODO maybe do this one step before?
				} else {
					raise_error(opening_brace == Symbols::Sleftparen ? ")" : "auto)", str(input_value));
					return(NULL);
				}
			} catch(...) {
				B_honor_indentation = prev_B_honor_indentation;
				throw;
			}
		} else {
#if 0
			if(input_value == Symbols::Srightparen || input_value == Symbols::Sautorightparen) { /* oops! */
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
AST::NodeT MathParser::parse_binary_operation(bool B_allow_args, int precedence_level) {
	struct AST::Symbol* associativity;
	bool B_visible_operator, B_unary_operator = false;
	//printf("level is %d, input is: %s\n", precedence_level, input_value->str().c_str());
	if(operator_precedence_list->empty_P(precedence_level))
		return(B_allow_args ? parse_application() : parse_value());
	/* special case for unary - */
	AST::NodeT result = (precedence_level == operator_precedence_list->minus_level && input_value == Symbols::Sdash) ? (B_unary_operator = true, Symbols::Szero) : parse_binary_operation(B_allow_args, operator_precedence_list->next_precedence_level(precedence_level));
	if(AST::NodeT actual_token = operator_precedence_list->match_operator(precedence_level, input_value, /*out*/associativity, /*out*/B_visible_operator)) {
		while(actual_token && actual_token != Symbols::SlessEOFgreater) {
			AST::NodeT operator_ = B_visible_operator ? consume() : Symbols::Sspace;
			if(input_value == Symbols::Srightparen || input_value == Symbols::Sautorightparen) { // premature end.
				if(!B_visible_operator)
					raise_error("<operand>", ")");
				return(B_unary_operator ? operator_ : makeApplication(operator_, result)); /* default to the binary operator */
			}
			AST::NodeT b = parse_binary_operation(B_allow_args, associativity != Symbols::Sright ? operator_precedence_list->next_precedence_level(precedence_level) : precedence_level);
			if(B_unary_operator && !b) // -nil
				return(operator_);
			result = operation(operator_, result, b);
			/* for right associative operations, the recursion will have consumed all the operators on that level and by virtue of that, the while loop will always stop after one iteration. */
			if(associativity == Symbols::Snone)
				break;
			actual_token = operator_precedence_list->match_operator(precedence_level, input_value, /*out*/associativity, /*out*/B_visible_operator);
		}
		return(result);
	} else
		return(result);
}
AST::NodeT MathParser::parse_expression(void) {
	if(operator_precedence_list)
		return parse_binary_operation(true, operator_precedence_list->next_precedence_level(-1));
	else
		return parse_application();
}
AST::NodeT MathParser::parse_argument(void) {
	assert(operator_precedence_list->apply_level != 0);
	return parse_binary_operation(false, operator_precedence_list->apply_level);
}
AST::NodeT MathParser::parse(OperatorPrecedenceList* operator_precedence_list) {
	this->operator_precedence_list = operator_precedence_list;
	AST::NodeT result = parse_expression();
	return(result);
}
AST::Cons* MathParser::parse_S_list_body(void) {
	if(input_value == Symbols::Srightparen || input_value == Symbols::SlessEOFgreater)
		return(NULL);
	else {
		AST::NodeT head;
		head = parse_S_Expression();
		return(makeCons(head, parse_S_list_body()));
	}
}
AST::Cons* MathParser::parse_S_list(bool B_consume_closing_brace) {
	bool prev_B_honor_indentation = B_honor_indentation;
	B_honor_indentation = false;
	try {
		AST::Cons* result = NULL;
		consume(Symbols::Sleftparen);
		/* TODO macros (if we want) */
		result = parse_S_list_body();
		if(B_consume_closing_brace)
			consume(Symbols::Srightparen);
		B_honor_indentation = prev_B_honor_indentation;
		return(result);
	} catch(...) {
		B_honor_indentation = prev_B_honor_indentation;
		throw;
	}
}
AST::NodeT MathParser::parse_S_Expression(void) {
	bool prev_B_honor_indentation = B_honor_indentation;
	B_honor_indentation = false;
	try {
		AST::NodeT result;
		/* TODO do this without tokenizing? How? */
		if(input_value == Symbols::Sleftparen) {
			result = parse_S_list(true);
		} else if(dynamic_cast<AST::Symbol*>(input_value) != NULL) {
			result = consume(); // & whitespace.
		} else {
			/* numbers, strings */
			if(input_value)
				result = consume();
			else {
				raise_error("<S_Expression>", "<junk>");
				result = NULL;
			}
			//parse_S_optional_whitespace();
		}
		B_honor_indentation = prev_B_honor_indentation;
		return(result);
	} catch(...) {
		B_honor_indentation = prev_B_honor_indentation;
		throw;
	}
}
AST::NodeT MathParser::parse_simple(const char* text, OperatorPrecedenceList* operator_precedence_list) {
	AST::NodeT result;
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
		return(NULL);
	}
}
void MathParser::parse_closing_brace(void) {
	// TODO auto)
	consume(Symbols::Srightparen);
}
void MathParser::enter_abstraction(AST::Symbol* name) {
	bound_symbols = AST::makeCons(name, bound_symbols);
}
void MathParser::leave_abstraction(AST::Symbol* name) {
	assert(bound_symbols && dynamic_cast<AST::Symbol*>(bound_symbols->head) == name);
	AST::NodeT n = bound_symbols->tail;
	bound_symbols->tail = NULL;
	bound_symbols = (AST::Cons*) n;
}
std::set<AST::Symbol*> MathParser::get_bound_symbols(const char* prefix) {
	std::set<AST::Symbol*> syms;
	for(AST::Cons* b = bound_symbols; b; b = (AST::Cons*) b->tail) {
		AST::Symbol* sym = (AST::Symbol*) b->head;
		if(strncmp(sym->name, prefix, strlen(prefix)) == 0) {
			syms.insert(sym);
		}
	}
	return(syms);
}
void MathParser::push(FILE* input_file, int line_number) {
	Scanner::push(input_file, line_number);
	consume();
}

};
#endif /* def OLD_PARSER */
