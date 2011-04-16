#include <iostream>
#include "AST/AST"
#include "AST/Symbol"
#include "Formatters/SExpression"

void to_S_Expression_CXX(AST::Node* node, std::ostream& output) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(node == NULL)
		output << "nil";
	else if(symbolNode)
		output << symbolNode->name;
	else if(consNode) {
		output << '(';
		for(; consNode; consNode = consNode->tail) {
			to_S_Expression_CXX(consNode->head, output);
			if(consNode->tail)
				output << ' ';
		}
		output << ')';
	} else { /* literal etc */
		output << node->str();
	}
}
void to_S_Expression(AST::Node* node, FILE* output_file) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(node);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(node);
	if(node == NULL)
		fprintf(output_file, "nil");
	else if(symbolNode)
		fprintf(output_file, "%s", symbolNode->name);
	else if(consNode) {
		fputc('(', output_file);
		for(; consNode; consNode = consNode->tail) {
			to_S_Expression(consNode->head, output_file);
			if(consNode->tail)
				fputc(' ', output_file);
		}
		fputc(')', output_file);
	} else { /* literal etc */
		std::string value = node->str();
		fprintf(output_file, "%s", value.c_str());
	}
}

