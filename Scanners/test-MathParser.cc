/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
		std::cerr << "error: input was " << source << "; expected " << expected_tree << " but got " << result->str() << std::endl;
		abort();
	}
}

int main() {
	test_expression("2+3⋅5", "(+ 2 (* 3 5))");
	test_expression("2⋅3+5", "(+ (* 2 3) 5)");
	test_expression("2⋅x+5", "(+ (* 2 x) 5)");
	test_expression("(2+3)⋅5", "(* (+ 2 3) 5)");
	test_expression("(sin 2+3)⋅5", "(* (+ (sin 2) 3) 5)");
	test_expression("(div 2 4 5+3)⋅5", "(* (+ (((div 2) 4) 5) 3) 5)");
	test_expression("\\x x", "(\\ x x)");
	test_expression("(\\x x) 1", "((\\ x x) 1)");
	test_expression("a⨯b", "(⨯ a b)");
	test_expression("a⃗⨯b⃗", "(⨯ a⃗ b⃗)");
	test_expression("2⋅f(x)", "(* 2 (f x))");
	test_expression("cos cos x", "((cos cos) x)"); // well, it doesn't know that cos is a function.
	return(0);
}
