#include <string.h>
#include <iostream>
#include <assert.h>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/LATEX"
#include "Evaluators/Evaluators"
#include "Scanners/OperatorPrecedenceList"
#include "Evaluators/Builtins"
#include "Formatters/UTFStateMachine"
#include "AST/Symbols"
#include "Numbers/Ratio"

namespace Formatters {
namespace LATEX {
using namespace Evaluators;

static inline bool maybe_print_opening_paren(std::ostream& output, int& position, AST::NodeT operator_, int precedence, int precedence_limit, bool B_brace_equal_levels) {
	if(operator_ == Symbols::Sslash) {
		output << "\\frac{";
		return(true);
	}
	if(precedence < precedence_limit) {
		output << "\\left(";
		return(true);
	} else if(precedence == precedence_limit) {
		/* the cases for +- are (all right-associated for a left-associative operator):
			1+(3-4)
			1+(3+4)
			1-(3-4)
			1-(3+4)
		*/
		// TODO what if they don't have the SAME associativity? Bad bad.
		if(B_brace_equal_levels)
			output << "\\left(";
		return(B_brace_equal_levels);
	} else
		return(false);
}
static inline void maybe_print_closing_paren(std::ostream& output, int& position, AST::NodeT operator_, bool B_parend) {
	if(operator_ == Symbols::Sslash) {
		output << "}"; // of frac
		return;
	}
	if(B_parend) {
		// the space is hopefully not visible. It's there because LATEX likes to special-case "!\right)".
		++position, output << " \\right)";
	}
}
static void print_text_LATEX(std::ostream& output, const char* text) {
	//bool B_mathrm = (symbolNode != Symbols::Sasteriskasterisk && symbolNode != Symbols::Scircumflex && symbolNode != Symbols::Sunderscore;
	bool B_mathrm = true;
	if(text[0] == '*' && text[1] == '*' && text[2] == 0)
		B_mathrm = false;
	else if(text[1] == 0) {
		if(text[0] == '^' || text[0] == '_' || text[0] == ' ') 
			B_mathrm = false;
	}
	if(B_mathrm)
		output << "\\mathrm{";
	const unsigned char* inputString = (const unsigned char*) text;
	if(inputString[0] == '\\') {
		output << "\\lambda ";
	} else if(inputString) {
		UTFStateMachine parser;
		const char* unmatched_beginning = (const char*) inputString;
		while(1) {
			const char* result;
			int input = *inputString;
			result = parser.get_final_result(input);
			if(result) {
				output << result;
				parser.reset();
				unmatched_beginning = (const char*) inputString;
			} else {
				if(parser.transition(input) == 0) { // was unknown.
					const char* unmatched_end = (const char*) inputString;
					for(const char* x = unmatched_beginning; x <= unmatched_end; ++x)
						if(*x)
							output << *x;
					unmatched_beginning = (const char*) unmatched_end + 1;
				} else
					unmatched_beginning = (const char*) inputString + 1;
				if(*inputString == 0)
					break;
				++inputString;
			}
		}
	}
	if(B_mathrm)
        	output << "}";
}
static void print_text(std::ostream& output, int& visible_position, const char* text, bool bForceParens) {
	if(bForceParens) {
		output << '(';
		print_text_LATEX(output, text);
		output << ')';
	} else
		print_text_LATEX(output, text);
	/*if(isalnum(*text) || ((text[0] == '"' || text[0] == '[') && text[1] != 0))
		output << text;
	else if(*text == '@')
		output << text;
	else if(*text == '\'' && *(text + 1) == 0) // unary operator
		output << '\'';
	else if(text[0] == '[' && text[1] == ']')
		output << "[]";*/
	for(; *text; ++text) {
		unsigned c = (unsigned) *text;
		if(c == 10)
			visible_position = 0;
		else if(c < 0x20)
			;
		else if(c < 0x80 || c >= 0xC0)
			++visible_position;
	}
}
static void print_text_raw(std::ostream& output, int& visible_position, const std::string& textStr, bool spaces) {
	const char* text = textStr.c_str();
	if(text[0] == '/' && text[1] == 0) {
		output << "}{";
		return;
	}
	if(spaces)
		output << "\\:";
	print_text_LATEX(output, text);
	if(spaces)
		output << "\\:";
	for(; *text; ++text) {
		unsigned c = (unsigned) *text;
		if(c == 10)
			visible_position = 0;
		else if(c < 0x20)
			;
		else if(c < 0x80 || c >= 0xC0)
			++visible_position;
	}
}
static inline void process_abstraction(Scanners::OperatorPrecedenceList* OPL, std::ostream& output, int& position, AST::NodeT node, int precedence_limit, bool B_paren_equal_levels) {
	int precedence = 0;
	bool B_parend = maybe_print_opening_paren(output, position, NULL, precedence, precedence_limit, B_paren_equal_levels);
	output << "\\frac{";
	while(abstraction_P(node)) {
		AST::NodeT parameter = get_abstraction_parameter(node);
		AST::NodeT body = get_abstraction_body(node);
		print_text_LATEX(output, "\\");
		print_text_raw(output, position, str(parameter), false);
		++position, output << ' ';
		node = body;
	}
	output << "}{";
	//print_CXX(OPL, output, position, parameter, precedence, false);
	print_CXX(OPL, output, position, node, precedence, false);
	output << "}";
	maybe_print_closing_paren(output, position, NULL, B_parend);
}

#include "Formatters/GenericPrinter"

}; /* end namespace LATEX */
}; /* end namespace Formatters */
