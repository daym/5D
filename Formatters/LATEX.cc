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

namespace Formatters {

static int apply_precedence_level = 2;

void limited_to_LATEX(AST::Node* node, std::ostream& output, int operator_precedence_limit) {
	using namespace Scanners;
	int operator_precedence;
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(symbolNode) {
		if(symbolNode == AST::intern("*"))
			output << "\\cdot ";
		else if(symbolNode == AST::intern("<="))
			output << "\\leq ";
		else if(symbolNode == AST::intern(">="))
			output << "\\geq ";
		else if(symbolNode == AST::intern("/="))
			output << "\\neq ";
		else
			output << node->str();
	} else if(consNode) {
		operator_precedence = get_operator_precedence(dynamic_cast<AST::Symbol*>(consNode->head));
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
			limited_to_LATEX(consNode->tail->head, output, operator_precedence);
			limited_to_LATEX(consNode->head, output, operator_precedence); /* operator */
			if(!consNode->tail->tail || consNode->tail->tail->tail)
				throw std::runtime_error("invalid binary math operation");
			limited_to_LATEX(consNode->tail->tail->head, output, operator_precedence);
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		} else { /* function call, maybe */
			operator_precedence = apply_precedence_level;
			if(operator_precedence > operator_precedence_limit)
				output << "\\left(";
			AST::Cons* args;
			limited_to_LATEX(consNode->head, output, operator_precedence_limit); /* FIXME precedence */
			for(args = consNode->tail; args; args = args->tail) {
				limited_to_LATEX(consNode->tail->head, output, operator_precedence_limit); /* FIXME precedence */
				if(args->tail)
					output << " ";
			}
			/* TODO what to do with more arguments, if that's even possible? */
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		}
	} else if(node) /* symbol etc */
		output << node->str();
	else
		output << "?";
}
void to_LATEX(AST::Node* node, std::ostream& output) {
	int operator_precedence_limit = 1000;
	limited_to_LATEX(node, output, operator_precedence_limit);
}

}; // end namespace Formatters
