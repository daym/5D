#include <stdio.h>
#include <iostream>
#include <string.h>
#include "Scanners/MathParser"

int main() {
	const char* buf = "2+3";
	using namespace Scanners;
	MathParser parser;
	AST::Node* result = parser.parse(fmemopen((void*) buf, strlen(buf), "r"));
	std::cout << result->str() << std::endl;
	return(0);
}
