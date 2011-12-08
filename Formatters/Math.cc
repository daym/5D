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
     * only print parens when necessary.
	 * do not print parens around subsequent applications (do  print parens if it's the first).
     * automatic indentation for "longer" expressions
*/

static void print_text(std::ostream& output, int& visible_position, const char* text, bool bForceParens) {
	if(bForceParens)
		output << '(' << text << ')';
	else
		output << text;
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
static inline bool maybe_print_opening_paren(std::ostream& output, int& position, int precedence, int precedence_limit, bool B_paren_equal_levels) {
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
static inline void maybe_print_closing_paren(std::ostream& output, int& position, bool B_parend) {
	if(B_parend) {
		++position, output << ')';
	}
}
void print_math_CXX(Scanners::OperatorPrecedenceList* OPL, std::ostream& output, int& position, AST::Node* node, int precedence_limit, bool B_paren_equal_levels) {
	/* the easiest way to think about this is that any and each node has a precedence level. 
	   The precedence level of the atoms is just very high. 
	   The reason it is not explicitly programmed that way is that there are a lot of temporary variables that are used in both precedence level detection and recursion and that would be much code duplication to do two. */
	node = repr(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);	
	if(node == NULL)
		print_text(output, position, "[]", false);
	else if(symbolNode) {
		int pl = OPL->get_operator_precedence(symbolNode);
		print_text(output, position, symbolNode->name, pl != -1);
	} else if(abstraction_P(node)) { /* abstraction */
		int precedence = 0;
		bool B_parend = maybe_print_opening_paren(output, position, precedence, precedence_limit, B_paren_equal_levels);
		AST::Node* parameter = get_abstraction_parameter(node);
		AST::Node* body = get_abstraction_body(node);
		++position, output << '\\';
		print_text_raw(output, position, str(parameter), false);
		//print_math_CXX(OPL, output, position, parameter, precedence, false);
		++position, output << ' ';
		print_math_CXX(OPL, output, position, body, precedence, false);
		maybe_print_closing_paren(output, position, B_parend);
	} else if(application_P(node)) { /* application */
		AST::Node* envelope = get_application_operator(node);
		if(!application_P(envelope))
			envelope = NULL;
		//if(operator_ && application_P(operator_)) // 2 for binary ops.
		AST::Node* operator_ = envelope ? get_application_operator(envelope) : NULL;
		AST::Symbol* operatorSymbol = dynamic_cast<AST::Symbol*>(repr(operator_)); 
		AST::Symbol* operatorAssociativity = NULL;
		int precedence = operatorSymbol ? OPL->get_operator_precedence_and_associativity(operatorSymbol, operatorAssociativity) : -1;
		if(precedence != -1 && application_P(envelope)) { // is a (binary) operator and the envelope is not a builtin (i.e. (+))
			bool B_parend = maybe_print_opening_paren(output, position, precedence, precedence_limit, B_paren_equal_levels);
			print_math_CXX(OPL, output, position, get_application_operand(envelope), precedence, operatorAssociativity != Symbols::Sleft);
			print_text_raw(output, position, operatorSymbol->str(), precedence < OPL->apply_level);
			////print_math_CXX(OPL, output, position, operator_, precedence, true); // ignored precedence
			print_math_CXX(OPL, output, position, get_application_operand(node), precedence, operatorAssociativity != Symbols::Sright);
			//print_text(output, position, operator_);
			//print_math_CXX(OPL, output, position, get_application_operand(node), precedence);
			maybe_print_closing_paren(output, position, B_parend);
		} else { // function application is fine and VERY greedy
			operatorAssociativity = Symbols::Sleft;
			precedence = OPL->apply_level;
			bool B_parend = maybe_print_opening_paren(output, position, precedence, precedence_limit, B_paren_equal_levels);
			operator_ = get_application_operator(node);
			operatorSymbol = dynamic_cast<AST::Symbol*>(operator_);
			int xprecedence = operatorSymbol ? OPL->get_operator_precedence(operatorSymbol) : -1;
			if(xprecedence != -1) { // incomplete binary operation
				++position, output << '(';
                                print_text_raw(output, position, operatorSymbol->str(), precedence < OPL->apply_level);
				++position, output << ')';
			} else
				print_math_CXX(OPL, output, position, operator_, precedence, operatorAssociativity != Symbols::Sleft);
			++position, output << ' ';
			print_math_CXX(OPL, output, position, get_application_operand(node), precedence, operatorAssociativity != Symbols::Sright);
			maybe_print_closing_paren(output, position, B_parend); // f.e. we now are at +, but came from *, i.e. 2*(3+5)
		}
	} else if(cons_P(node)) {
		output << "[";
		print_math_CXX(OPL, output, position, ((AST::Cons*)node)->head, 0, false);
		for(AST::Cons* vnode = Evaluators::evaluateToCons(((AST::Cons*)node)->tail); vnode; vnode = Evaluators::evaluateToCons(vnode->tail)) {
			output << " ";
			print_math_CXX(OPL, output, position, dynamic_cast<AST::Cons*>(vnode)->head, 0, false);
		}
		output << "]";
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		std::string value = str(node);
		int pl = OPL->get_operator_precedence(AST::symbolFromStr(value.c_str()));
		print_text(output, position, value.c_str(), pl != -1); // , pl < OPL->apply_level && pl != -1);
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

