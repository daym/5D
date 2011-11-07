#include <string.h>
#include <iostream>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/Math"
#include "Evaluators/Evaluators"
#include "Scanners/OperatorPrecedenceList"

namespace Formatters {
using namespace Evaluators;

// TODO make this a combination of LATEX and S Expression output.
/* TODO Features: 
     * only print braces when necessary.
	 * do not print braces around subsequent applications (do  print braces if it's the first).
     * automatic indentation for "longer" expressions
*/

static void print_text(std::ostream& output, int& visible_position, const char* text) {
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
void print_math_CXX(Scanners::OperatorPrecedenceList* OPL, std::ostream& output, int& position, AST::Node* node, int precedence_limit) {
	AST::Operation* operation = dynamic_cast<AST::Operation*>(node);
	if(operation) {
		AST::Node* n = operation->repr(NULL/*FIXME*/);
		if(n)
			node = n;
	}
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);	
	if(node == NULL)
		print_text(output, position, "nil");
	else if(symbolNode)
		print_text(output, position, symbolNode->name);
	else if(abstraction_P(node)) { /* abstraction */
		int index = 0;
		output << '(';
		++position;
		for(; consNode; ++index, consNode = consNode->tail) {
			print_math_CXX(OPL, output, position, consNode->head, precedence_limit);
			if(consNode->tail) {
				output << ' ';
				++position;
			}
		}
		output << ')';
		++position;
	} else if(application_P(node)) { /* application */
		AST::Node* operator_ = get_application_operator(node);
		//if(operator_ && application_P(operator_)) // 2 for binary ops.
		//	operator_ = get_application_operator(operator_);
		AST::Symbol* operatorSymbol = dynamic_cast<AST::Symbol*>(operator_); 
		int precedence = operatorSymbol ? OPL->get_operator_precedence(operatorSymbol) : -1;
		if(precedence != -1) { // is a (binary) operator
			if(precedence < precedence_limit) // f.e. we now are at +, but came from *, i.e. 2*(3+5)
				output << '(';
			print_math_CXX(OPL, output, position, operator_, precedence); // ignored precedence
			output << ' ';
			print_math_CXX(OPL, output, position, get_application_operand(node), precedence);
			//print_text(output, position, operator_);
			//print_math_CXX(OPL, output, position, get_application_operand(node), precedence);
			if(precedence < precedence_limit) // f.e. we now are at +, but came from *, i.e. 2*(3+5)
				output << ')';
		} else { // function application is fine and VERY greedy
				print_math_CXX(OPL, output, position, operator_, precedence); // ignored precedence
				output << ' ';
				print_math_CXX(OPL, output, position, get_application_operand(node), OPL->apply_level);
		}
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		std::string value = node->str();
		print_text(output, position, value.c_str());
	}
}
void print_math(Scanners::OperatorPrecedenceList* OPL, FILE* output_file, int position, int indentation, AST::Node* node) {
	std::stringstream sst;
	std::string value;
	print_math_CXX(OPL, sst, position, node, 0);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

}; /* end namespace formatters */

