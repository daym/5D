#include <stdio.h>
#include <string.h>
#include "Scanners/MathParser"

int main() {
	const char* buf = "2+3";
	using namespace Scanners;
	MathParser parser;
	parser.parse(fmemopen((void*) buf, strlen(buf), "r"));
	return(0);
}
