/*
4D vector analysis program
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

namespace Formatters {

void limited_to_LATEX(AST::Node* node, std::ostream& output, int operator_precedence_limit) {
	using namespace Scanners;
	int operator_precedence;
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(symbolNode) {
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
	} else if(consNode) {
		/* ((- 3) 2)   => 3-2
		 or (0- 3)      => -3 */
		AST::Cons* innerCons = dynamic_cast<AST::Cons*>(consNode->head);
		operator_precedence = get_operator_precedence(dynamic_cast<AST::Symbol*>(innerCons ? innerCons->head : NULL));
		/*if(operator_precedence == -1)
			operator_precedence = apply_precedence_level;*/
		if(operator_precedence != -1 && consNode->head == AST::intern("/")) { /* fraction */
			output << "{\\frac{";
			limited_to_LATEX(consNode->tail->head, output, operator_precedence);
			output << "}{";
			if(!consNode->tail->tail || consNode->tail->tail->tail)
				throw std::runtime_error("invalid fraction");
			limited_to_LATEX(consNode->tail->tail->head, output, operator_precedence);
			output << "}}";
		} else if(operator_precedence != -1) { /* actual binary math operator */
			if(operator_precedence > operator_precedence_limit)
				output << "\\left(";
			limited_to_LATEX(innerCons->tail->head, output, operator_precedence);
			limited_to_LATEX(innerCons->head, output, operator_precedence); /* operator */
			if(!consNode->tail || !innerCons || !innerCons->head || !innerCons->tail || !innerCons->tail->head)
				throw std::runtime_error("invalid binary math operation");
			limited_to_LATEX(consNode->tail->head, output, operator_precedence-1);
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		} else if(consNode->head == AST::intern("\\")) {
			operator_precedence = apply_precedence_level; /* TODO do not hardcode. */
			if(operator_precedence > operator_precedence_limit)
				output << "\\left(";
			AST::Cons* args;
			limited_to_LATEX(consNode->head, output, operator_precedence); /* FIXME precedence */
			//output << "\\:";
			for(args = consNode->tail; args; args = args->tail) { /* eew */
				limited_to_LATEX(args->head, output, operator_precedence); /* FIXME precedence */
				if(args->tail)
					output << "\\:";
			}
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		} else { /* function call, maybe */
			operator_precedence = apply_precedence_level;
			if(operator_precedence > operator_precedence_limit)
				output << "\\left(";
			AST::Cons* args;
			limited_to_LATEX(consNode->head, output, operator_precedence); /* FIXME precedence */
			output << "\\:";
			for(args = consNode->tail; args; args = args->tail) { /* actually just one, so didn't pay much attention to operator precedence limit below */
				limited_to_LATEX(args->head, output, operator_precedence-1); /* FIXME precedence */
				if(args->tail)
					output << "\\:";
			}
			/* TODO what to do with more arguments, if that's even possible? */
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		}
	} else if(node)
		output << "\\mathrm{" << node->str() << "}";
	else
		output << "nil";
}
void to_LATEX(AST::Node* node, std::ostream& output) {
	int operator_precedence_limit = 1000;
	limited_to_LATEX(node, output, operator_precedence_limit);
	limited_to_LATEX(node, std::cout, operator_precedence_limit);
}

}; // end namespace Formatters
