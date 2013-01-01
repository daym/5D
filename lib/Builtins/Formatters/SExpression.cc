#include <string.h>
#include <iostream>
#include <sstream>
#include <5D/Operations>
#include "Values/Values"
#include "Formatters/SExpression"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Numbers/Ratio"

namespace Formatters {
using namespace Evaluators;
using namespace Values;

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
void print_S_Expression_CXX(std::ostream& output, int& position, int indentation, NodeT node) {
	bool B_split_cons_items = true;
	const char* symbolName;
	if(node == NULL)
		print_text(output, position, "nil");
	else if((symbolName = get_symbol1_name(node)) != NULL)
		print_text(output, position, symbolName);
	else if(cons_P(node)) {
		int index = 0;
		output << '(';
		++position;
		for(; node; ++index, node = Evaluators::evaluateToCons(get_cons_tail(node))) {
			print_S_Expression_CXX(output, position, indentation, get_cons_head(node));
			if(get_cons_tail(node)) {
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
		print_S_Expression_CXX(output, position, indentation, get_application_operator(node));
		output << ' ';
		++position;
		print_S_Expression_CXX(output, position, indentation, get_application_operand(node));
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
		print_S_Expression_CXX(output, position, indentation, get_abstraction_parameter(node));
		output << ' ';
		indentation = position;
		output << std::endl;
		print_indentation(output, indentation);
		position = indentation;
		++position;
		print_S_Expression_CXX(output, position, indentation, get_abstraction_body(node));
		output << ')';
		++position;
		indentation = prevIndentation;
	} else { /* literal etc */
		/* this especially matches BuiltinOperators which will return their builtin name */
		std::string value = str(node);
		print_text(output, position, value.c_str());
	}
}
void print_S_Expression(FILE* output_file, int position, int indentation, NodeT node) {
	std::stringstream sst;
	std::string value;
	node = repr(node);
	print_S_Expression_CXX(sst, position, indentation, node);
	value = sst.str();
	fprintf(output_file, "%s", value.c_str());
}

BEGIN_PROC_WRAPPER(printS, 4, symbolFromStr("printS!"), )
	FILE* outputFile = (FILE*) FNARG_FETCH(pointer);
	int position = FNARG_FETCH(int);
	int indentation = FNARG_FETCH(int);
	NodeT node = FNARG_FETCH(node);
	Formatters::print_S_Expression(outputFile, position, indentation, node);
	return(MONADIC(nil));
END_PROC_WRAPPER

}; /* end namespace formatters */

