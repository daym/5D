#include <string.h>
#include <iostream>
#include <assert.h>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/Math"
#include "Evaluators/Evaluators"
#include "Scanners/OperatorPrecedenceList"
#include "Evaluators/Builtins"
#include "Numbers/Ratio"

namespace Formatters {
namespace Math {
using namespace Evaluators;

/* TODO Features: 
     * automatic indentation for "longer" expressions
*/

static void print_text(std::ostream& output, int& visible_position, const char* text, bool bForceParens) {
	if(bForceParens)
		output << '(' << text << ')';
	else
		output << text;
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
	if(spaces)
		output << ' ';
	output << text;
	if(spaces)
		output << ' ';
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
#define XN(x) OPL->next_precedence_level(x)
static inline bool maybe_print_opening_paren(std::ostream& output, int& position, AST::NodeT operator_, int precedence, int precedence_limit, bool B_paren_equal_levels) {
	if(precedence < precedence_limit) {
		++position, output << '(';
		return(true);
	} else if(precedence == precedence_limit) {
		/* the cases for +- are (all right-associated for a left-associative operator):
			1+(3-4)
			1+(3+4)
			1-(3-4)
			1-(3+4)
		*/
		// TODO what if they don't have the SAME associativity? Bad bad.
		if(B_paren_equal_levels)
			++position, output << '(';
		return(B_paren_equal_levels);
	} else
		return(false);
}
static inline void maybe_print_closing_paren(std::ostream& output, int& position, AST::NodeT operator_, bool B_parend) {
	if(B_parend) {
		++position, output << ')';
	}
}
static inline void process_abstraction(Scanners::OperatorPrecedenceList* OPL, std::ostream& output, int& position, AST::NodeT node, int precedence_limit, bool B_paren_equal_levels) {
	int precedence = 0;
	bool B_parend = maybe_print_opening_paren(output, position, NULL, precedence, precedence_limit, B_paren_equal_levels);
	AST::NodeT parameter = get_abstraction_parameter(node);
	AST::NodeT body = get_abstraction_body(node);
	++position, output << '\\';
	print_CXX(OPL, output, position, parameter, precedence_limit /* does not matter */, false);
	++position, output << ' ';
	print_CXX(OPL, output, position, body, precedence, false);
	maybe_print_closing_paren(output, position, NULL, B_parend);
}

#include "Formatters/GenericPrinter"

}; /* end namespace Math */
}; /* end namespace Formatters */

