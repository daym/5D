#include <string.h>
#include <iostream>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/Math"

namespace Formatters {

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
static void print_indentation(std::ostream& output, int indentation) {
	for(; indentation > 0; --indentation)
		output << ' ';
}
void print_math_CXX(std::ostream& output, int& position, int indentation, AST::Node* node) {
	bool B_split_cons_items = true;
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
	else if(consNode) {
		int index = 0;
		output << '(';
		++position;
		for(; consNode; ++index, consNode = consNode->tail) {
			print_math_CXX(output, position, indentation, consNode->head);
			if(consNode->tail) {
				if(B_split_cons_items && index >= 1) {
					if(index == 1) {
						output << std::endl;
						print_indentation(output, indentation);
						position = indentation;
					}
				} else {
					output << ' ';
					++position;
				}
				if(B_split_cons_items && index == 0)
					indentation = position;
			}
		}
		output << ')';
		++position;
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		std::string value = node->str();
		print_text(output, position, value.c_str());
	}
}
void print_math(FILE* output_file, int position, int indentation, AST::Node* node) {
	std::stringstream sst;
	std::string value;
	print_math_CXX(sst, position, indentation, node);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

}; /* end namespace formatters */

