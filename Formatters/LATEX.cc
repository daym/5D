/*
5D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <stdexcept>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/LATEX"
#include "Scanners/MathParser"
#include "Formatters/UTFStateMachine"
#include "Evaluators/Builtins"

namespace Formatters {
using namespace Evaluators;

static inline bool maybe_print_opening_brace(std::ostream& output, int precedence, int precedence_limit, bool B_brace_equal_levels) {
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
// FIXME application, abstraction
void limited_to_LATEX(Scanners::OperatorPrecedenceList* operator_precedence_list, AST::Node* node, std::ostream& output, int operator_precedence_limit, bool B_brace_equal_levels) {
	using namespace Scanners;
	int operator_precedence;
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	AST::SymbolReference* symbolReference = dynamic_cast<AST::SymbolReference*>(node);
	if(symbolReference)
		symbolNode = symbolReference->symbol;
	if(symbolNode) {
		bool B_mathrm = symbolNode != AST::intern("**") && symbolNode != AST::intern("^") && symbolNode != AST::intern("_");
		// && symbolNode != AST::intern("<") && symbolNode != AST::intern(">") && symbolNode != AST::intern("≤") && symbolNode != AST::intern("≥");
		if(B_mathrm)
			output << "\\mathrm{";
		std::string text = symbolNode->str();
		const unsigned char* inputString = (const unsigned char*) text.c_str();
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
		} /*else
			output << text;*/
		// TODO output << "\\operatorname{" << node->str() << "}"; // "\\math{" << node->str() << "}";
		if(B_mathrm)
			output << "}";
	} else if(application_P(node) || abstraction_P(node)) {
		/* ((- 3) 2)   => 3-2
		 or (0- 3)      => -3 */
		AST::Node* innerApplication = application_P(node) ? dynamic_cast<AST::Application*>(get_application_operator(node)) : NULL;
		AST::Symbol* operatorAssociativity = NULL;
		operator_precedence = operator_precedence_list ? operator_precedence_list->get_operator_precedence_and_associativity(dynamic_cast<AST::Symbol*>(innerApplication ? get_application_operator(innerApplication) : NULL), operatorAssociativity) : -1;
		/*if(operator_precedence == -1)
			operator_precedence = apply_precedence_level;*/
		if(abstraction_P(node)) {
			operator_precedence = 9999; // FIXME lambda_precedence_level;
			bool B_braced = maybe_print_opening_brace(output, operator_precedence, operator_precedence_limit, B_brace_equal_levels);
			AST::Node* node2 = node;
			output << "\\frac{";
			while(abstraction_P(node2)) {
				AST::Node* parameter = get_abstraction_parameter(node2);
				limited_to_LATEX(operator_precedence_list, AST::intern("\\"), output, operator_precedence, false); /* lambda */
				/* parameter */
				limited_to_LATEX(operator_precedence_list, parameter, output, operator_precedence, false); /* FIXME precedence */
				node2 = get_abstraction_body(node2);
			}
			output << "}{";
			/* body */
			limited_to_LATEX(operator_precedence_list, node2, output, operator_precedence, true/*assoc is right*/); /* FIXME precedence */
			//output << "\\:";
			output << "}";
			if(B_braced)
				output << "\\right)";
		} else if(operator_precedence != -1 && innerApplication && get_application_operator(innerApplication) == AST::intern("/")) { /* fraction */
			output << "{\\frac{";
			limited_to_LATEX(operator_precedence_list, get_application_operand(innerApplication), output, operator_precedence, false);
			output << "}{";
			//throw std::runtime_error("invalid fraction");
			limited_to_LATEX(operator_precedence_list, get_application_operand(node), output, operator_precedence, false/*TODO*/);
			output << "}}";
		} else if(operator_precedence != -1) { /* actual binary math operator */
			bool B_braced = maybe_print_opening_brace(output, operator_precedence, operator_precedence_limit, B_brace_equal_levels);
			limited_to_LATEX(operator_precedence_list, get_application_operand(innerApplication), output, operator_precedence, operatorAssociativity != AST::intern("left"));
			limited_to_LATEX(operator_precedence_list, get_application_operator(innerApplication), output, operator_precedence, true);
			//	throw std::runtime_error("invalid binary math operation");
			limited_to_LATEX(operator_precedence_list, get_application_operand(node), output, operator_precedence, operatorAssociativity != AST::intern("right")); /* operator */
			if(B_braced)
				output << "\\right)";
		} else { /* function call */
			operator_precedence = operator_precedence_list->apply_level;
			bool B_braced = maybe_print_opening_brace(output, operator_precedence, operator_precedence_limit, B_brace_equal_levels);
			AST::Cons* args;
			limited_to_LATEX(operator_precedence_list, get_application_operator(node), output, operator_precedence, true); /* FIXME precedence */
			output << "\\:";
			limited_to_LATEX(operator_precedence_list, get_application_operand(node), output, operator_precedence, false); /* FIXME precedence */
			/* TODO what to do with more arguments, if that's even possible? */
			if(B_braced)
				output << "\\right)";
		}
		// TODO cons etc
	} else if(node)
		output << "\\mathrm{" << str(node) << "}";
	else
		output << "nil";
}
void to_LATEX(Scanners::OperatorPrecedenceList* operator_precedence_list, AST::Node* node, std::ostream& output) {
	int operator_precedence_limit = 0; // FIXME -1 ?
	limited_to_LATEX(operator_precedence_list, node, output, operator_precedence_limit, true);
	//limited_to_LATEX(node, std::cout, operator_precedence_limit);
}

}; // end namespace Formatters
