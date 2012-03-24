#include <string.h>
#include <iostream>
#include <sstream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/SExpression"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Numbers/Ratio"

namespace Formatters {
using namespace Evaluators;

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
void print_S_Expression_CXX(std::ostream& output, int& position, int indentation, AST::Node* node) {
	bool B_split_cons_items = true;
	node = repr(node);
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
		for(; consNode; ++index, consNode = Evaluators::evaluateToCons(consNode->tail)) {
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
	} else if(application_P(node)) {
		/*if(B_split_cons_items) {
			//indentation = position;
			output << std::endl;
			print_indentation(output, indentation);
			position = indentation;
		} else {
			output << ' ';
			++position;
		}*/
		output << '(';
		++position;
		print_S_Expression_CXX(output, position, indentation, AST::get_application_operator(node));
		output << ' ';
		++position;
		print_S_Expression_CXX(output, position, indentation, AST::get_application_operand(node));
		output << ')';
		++position;
		/*if(B_split_cons_items) {
			indentation = position;
			output << std::endl;
			print_indentation(output, indentation);
			position = indentation;
		} else {
			output << ' ';
			++position;
		}*/
	} else if(abstraction_P(node)) {
		int prevIndentation = indentation;
		output << "(\\";
		++position;
		++position;
		print_S_Expression_CXX(output, position, indentation, AST::get_abstraction_parameter(node));
		output << ' ';
		indentation = position;
		output << std::endl;
		print_indentation(output, indentation);
		position = indentation;
		++position;
		print_S_Expression_CXX(output, position, indentation, AST::get_abstraction_body(node));
		output << ')';
		++position;
		indentation = prevIndentation;
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		std::string value = str(node);
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

