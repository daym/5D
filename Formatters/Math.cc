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

namespace Formatters {
using namespace Evaluators;

// TODO make this a combination of LATEX and S Expression output.
/* TODO Features: 
     * only print braces when necessary.
	 * do not print braces around subsequent applications (do  print braces if it's the first).
     * automatic indentation for "longer" expressions
*/

static void print_text(std::ostream& output, int& visible_position, const char* text) {
	if(isalnum(*text))
		output << text;
	else if(*text == '@')
		output << text;
	else if(*text == '\'' && *(text + 1) == 0) // unary operator
		output << '\'';
	else
		output << '(' << text << ')';
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
static void print_text_raw(std::ostream& output, int& visible_position, const std::string& textStr) {
	const char* text = textStr.c_str();
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
#define XN(x) OPL->next_precedence_level(x)
static inline bool maybe_print_opening_brace(std::ostream& output, int& position, int precedence, int precedence_limit, bool B_brace_equal_levels) {
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
		if(B_brace_equal_levels)
			++position, output << '(';
		return(B_brace_equal_levels);
	} else
		return(false);
}
void print_math_CXX(Scanners::OperatorPrecedenceList* OPL, std::ostream& output, int& position, AST::Node* node, int precedence_limit, bool B_brace_equal_levels) {
	/* the easiest way to think about this is that any and each node has a precedence level. 
	   The precedence level of the atoms is just very high. 
	   The reason it is not explicitly programmed that way is that there are a lot of temporary variables that are used in both precedence level detection and recursion and that would be much code duplication to do two. */
	AST::Operation* operation = dynamic_cast<AST::Operation*>(node);
	if(operation) {
		AST::Node* n = operation->repr(NULL/*FIXME*/);
		if(n)
			node = n;
	}
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);	
	if(node == NULL)
		print_text(output, position, "nil");
	else if(symbolNode)
		print_text(output, position, symbolNode->name);
	else if(abstraction_P(node)) { /* abstraction */
		int precedence = 0;
		bool B_braced = maybe_print_opening_brace(output, position, precedence, precedence_limit, B_brace_equal_levels);
		AST::Node* parameter = get_abstraction_parameter(node);
		AST::Node* body = get_abstraction_body(node);
		++position, output << '\\';
		print_math_CXX(OPL, output, position, parameter, precedence, false);
		++position, output << ' ';
		print_math_CXX(OPL, output, position, body, precedence, false);
		if(B_braced)
			++position, output << ')';
	} else if(application_P(node)) { /* application */
		AST::Node* envelope = get_application_operator(node);
		if(!application_P(envelope))
			envelope = NULL;
		//if(operator_ && application_P(operator_)) // 2 for binary ops.
		AST::Node* operator_ = envelope ? get_application_operator(envelope) : NULL;
		AST::Symbol* operatorSymbol = dynamic_cast<AST::Symbol*>(operator_); 
		AST::Symbol* operatorAssociativity = NULL;
		int precedence = operatorSymbol ? OPL->get_operator_precedence_and_associativity(operatorSymbol, operatorAssociativity) : -1;
		if(precedence != -1 && application_P(envelope)) { // is a (binary) operator and the envelope is not a builtin (i.e. (+))
			bool B_braced = maybe_print_opening_brace(output, position, precedence, precedence_limit, B_brace_equal_levels);
			print_math_CXX(OPL, output, position, get_application_operand(envelope), precedence, operatorAssociativity != AST::intern("left"));
			print_text_raw(output, position, operatorSymbol->str());
			////print_math_CXX(OPL, output, position, operator_, precedence, true); // ignored precedence
			print_math_CXX(OPL, output, position, get_application_operand(node), precedence, operatorAssociativity != AST::intern("right"));
			//print_text(output, position, operator_);
			//print_math_CXX(OPL, output, position, get_application_operand(node), precedence);
			if(B_braced)
				++position, output << ')';
		} else { // function application is fine and VERY greedy
			operatorAssociativity = AST::intern("left");
			precedence = OPL->apply_level;
			bool B_braced = maybe_print_opening_brace(output, position, precedence, precedence_limit, B_brace_equal_levels);
			operator_ = get_application_operator(node);
			operatorSymbol = dynamic_cast<AST::Symbol*>(operator_);
			int xprecedence = operatorSymbol ? OPL->get_operator_precedence(operatorSymbol) : -1;
			if(xprecedence != -1) { // incomplete binary operation
				++position, output << '(';
                                print_text_raw(output, position, operatorSymbol->str());
				++position, output << ')';
			} else
				print_math_CXX(OPL, output, position, operator_, precedence, operatorAssociativity != AST::intern("left"));
			++position, output << ' ';
			print_math_CXX(OPL, output, position, get_application_operand(node), precedence, operatorAssociativity != AST::intern("right"));
			if(B_braced) // f.e. we now are at +, but came from *, i.e. 2*(3+5)
				++position, output << ')';
		}
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		/* FIXME braces for these? */
		std::string value = str(node);
		print_text_raw(output, position, value.c_str());
	}
}
void print_math(Scanners::OperatorPrecedenceList* OPL, FILE* output_file, int position, int indentation, AST::Node* node) {
	std::stringstream sst;
	std::string value;
	print_math_CXX(OPL, sst, position, node, 0, false);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

}; /* end namespace formatters */

