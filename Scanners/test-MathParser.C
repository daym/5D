#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "Scanners/MathParser"

void test_expression(const char* source, const char* expected_tree) {
	const char* buf = source;
	using namespace Scanners;
	MathParser parser;
	//std::cout << source << std::endl;
	AST::Node* result = parser.parse(fmemopen((void*) buf, strlen(buf), "r"));
	if(result->str() != expected_tree) {
		std::cerr << result->str() << std::endl;
		abort();
	}
}

int main() {
	test_expression("2+3⋅5", "(+ 2 (* 3 5))");
	test_expression("2⋅3+5", "(+ (* 2 3) 5)");
	test_expression("2⋅x+5", "(+ (* 2 x) 5)");
	test_expression("(2+3)⋅5", "(* (+ 2 3) 5)");
	test_expression("(sin 2+3)⋅5", "(* (+ (apply sin 2) 3) 5)");
	test_expression("(div 2 4 5+3)⋅5", "(* (+ (apply (apply (apply div 2) 4) 5) 3) 5)");
	return(0);
}
