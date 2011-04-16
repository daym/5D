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
void print_S_Expression_CXX(std::ostream& output, int indentation, AST::Node* node) {
	bool B_split_cons_items = true;
	int visible_position = 0;
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(node == NULL)
		print_text(output, visible_position, "nil");
	else if(symbolNode)
		print_text(output, visible_position, symbolNode->name);
	else if(consNode) {
		bool B_first = true;
		output << '(';
		++visible_position;
		for(; consNode; consNode = consNode->tail) {
			print_S_Expression_CXX(output, indentation, consNode->head);
			if(consNode->tail) {
				if(B_split_cons_items) {
					if(B_first) {
						B_first = false;
						indentation += visible_position;
					}
					output << std::endl;
					print_indentation(output, indentation);
					visible_position = indentation;
				} else {
					output << ' ';
					++visible_position;
				}
			}
		}
		output << ')';
		++visible_position;
	} else { /* literal etc */
		std::string value = node->str();
		print_text(output, visible_position, value.c_str());
	}
}
void print_S_Expression(FILE* output_file, int indentation, AST::Node* node) {
	std::stringstream sst;
	std::string value;
	print_S_Expression_CXX(sst, indentation, node);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

}; /* end namespace formatters */

