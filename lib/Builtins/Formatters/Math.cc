#include <string.h>
#include <iostream>
#include <assert.h>
#include <sstream>
#include <5D/Operations>
#include "Values/Values"
#include "Formatters/Math"
#include "Evaluators/Evaluators"
#include "Scanners/LOperatorPrecedenceList"
#include "Evaluators/Builtins"
#include "Numbers/Ratio"

namespace Formatters {
namespace Math {
using namespace Evaluators;
using namespace Values;

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
static inline bool maybe_print_opening_paren(std::ostream& output, int& position, NodeT operator_, int precedence, int precedence_limit, bool B_paren_equal_levels) {
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
static inline void maybe_print_closing_paren(std::ostream& output, int& position, NodeT operator_, bool B_parend) {
	if(B_parend) {
		++position, output << ')';
	}
}
void print_CXX(Scanners::LOperatorPrecedenceList* OPL, std::ostream& output, int& position, Values::NodeT node, int precedence_limit, bool B_paren_equal_levels);
static inline void process_abstraction(Scanners::LOperatorPrecedenceList* OPL, std::ostream& output, int& position, NodeT node, int precedence_limit, bool B_paren_equal_levels) {
	int precedence = 0;
	bool B_parend = maybe_print_opening_paren(output, position, NULL, precedence, precedence_limit, B_paren_equal_levels);
	NodeT parameter = get_abstraction_parameter(node);
	NodeT body = get_abstraction_body(node);
	++position, output << '\\';
	print_CXX(OPL, output, position, parameter, precedence_limit /* does not matter */, false);
	++position, output << ' ';
	print_CXX(OPL, output, position, body, precedence, false);
	maybe_print_closing_paren(output, position, NULL, B_parend);
}

#include "Formatters/GenericPrinter"

BEGIN_PROC_WRAPPER(printMath, 5, symbolFromStr("printMath!"), )
	NodeT OPL = FNARG_FETCH(node);
	FILE* outputFile = (FILE*) FNARG_FETCH(pointer);
	int position = FNARG_FETCH(int);
	int indentation = FNARG_FETCH(int);
	NodeT node = FNARG_FETCH(node);
	print(OPL, outputFile, position, indentation, node);
	return(MONADIC(nil));
END_PROC_WRAPPER

}; /* end namespace Math */
}; /* end namespace Formatters */
/* TODO U+2063 invisible separator or invisible comma is intended for use in index expressions and other mathematical notation where two adjacent variables form a list and are not implicitly multiplied. */
/* non-displaying U+2061invisible times */
/* use of U+2062 function application for an implied function dependence as in f(x + y)*/
