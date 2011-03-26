/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include <assert.h>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/LATEX"
#include "Scanners/MathParser"

namespace Formatters {

void limited_to_LATEX(AST::Node* node, std::ostream& output, int operator_precedence_limit) {
	using namespace Scanners;
	int operator_precedence;
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(symbolNode) {
		if(symbolNode == AST::intern("*"))
			output << "\\cdot";
		else
			output << node->str();
	} else if(consNode) {
		operator_precedence = get_operator_precedence(dynamic_cast<AST::Symbol*>(consNode->head));
		if(operator_precedence != -1 && consNode->head == AST::intern("/")) { /* fraction */
			output << "{\\frac{";
			limited_to_LATEX(consNode->tail->head, output, operator_precedence);
			output << "}{";
			assert(consNode->tail->tail);
			assert(!consNode->tail->tail->tail);
			limited_to_LATEX(consNode->tail->tail->head, output, operator_precedence);
			output << "}}";
		} else if(operator_precedence != -1) { /* actual binary math operator */
			if(operator_precedence > operator_precedence_limit)
				output << "\\left(";
			limited_to_LATEX(consNode->tail->head, output, operator_precedence);
			limited_to_LATEX(consNode->head, output, operator_precedence); /* operator */
			assert(consNode->tail->tail);
			assert(!consNode->tail->tail->tail);
			limited_to_LATEX(consNode->tail->tail->head, output, operator_precedence);
			if(operator_precedence > operator_precedence_limit)
				output << "\\right)";
		} else {
			/* TODO */
			assert(0);
		}
	} else if(node)
		output << node->str();
	else
		output << "?";
}
void to_LATEX(AST::Node* node, std::ostream& output) {
	int operator_precedence_limit = 1000;
	limited_to_LATEX(node, output, operator_precedence_limit);
}

}; // end namespace Formatters
