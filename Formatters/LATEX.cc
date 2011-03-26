/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include "AST/AST"
#include "Formatters/LATEX"
#include "Scanners/MathParser"

namespace Formatters {

void to_LATEX(AST::Node* node, std::ostream& output) {
	using namespace Scanners;
	int operator_precedence;
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	operator_precedence = get_operator_precedence(symbolNode);
	if(node)
		output << node->str();
	else
		output << "?";
}

}; // end namespace Formatters
