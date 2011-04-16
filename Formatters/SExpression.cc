#include <string.h>
#include <iostream>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/SExpression"

namespace Formatters {

static void print_text(std::ostream& output, int& visible_position, const char* text) {
	output << text;
	visible_position += strlen(text); /* TODO UTF-8 decode and ignore compositing characters completely */
}
static void print_indentation(std::ostream& output, int indentation) {
	for(; indentation > 0; --indentation)
		output << ' ';
}
void print_S_Expression_CXX(std::ostream& output, int& position, int indentation, AST::Node* node) {
	bool B_split_cons_items = true;
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
			print_S_Expression_CXX(output, position, indentation, consNode->head);
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
		std::string value = node->str();
		print_text(output, position, value.c_str());
	}
}
void print_S_Expression(FILE* output_file, int position, int indentation, AST::Node* node) {
	std::stringstream sst;
	std::string value;
	print_S_Expression_CXX(sst, position, indentation, node);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

}; /* end namespace formatters */

