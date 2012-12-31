/*
5D programming language
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
#include "Scanners/OperatorPrecedenceList"
#include "Evaluators/Builtins"
#include "AST/Symbols"

namespace GUI {
bool interrupted_P(void) {
	return(false);
}
};

using namespace Evaluators;

void test_expression(const char* source, const char* expected_tree) {
	const char* buf = source;
	using namespace Scanners;
	MathParser parser;
	//printf("==== %s\n", source);
	//printf("E====\n");
	//std::cout << source << std::endl;
	parser.push(fmemopen((void*) buf, strlen(buf), "r"), 0);
	try {
		NodeT result = parser.parse(new OperatorPrecedenceList(), Symbols::SlessEOFgreater);
		if(str(result) != expected_tree) {
			std::cerr << "error: input was " << source << "; expected " << expected_tree << " but got " << str(result) << std::endl;
			abort();
		}
	} catch(Scanners::ParseException e) {
		std::cerr << "error: input was " << source << "; expected " << expected_tree << " but got exception " << e.what() << std::endl;
		throw;
	}
}
void test_error_expression(const char* source, const char* expected_text) {
	const char* buf = source;
	using namespace Scanners;
	MathParser parser;
	//printf("==== %s\n", source);
	//printf("E====\n");
	//std::cout << source << std::endl;
	parser.push(fmemopen((void*) buf, strlen(buf), "r"), 0);
	try {
		parser.parse(new OperatorPrecedenceList(), Symbols::SlessEOFgreater);
		abort();
	} catch(Scanners::ParseException e) {
		if(strstr(e.what(), expected_text) == NULL) {
			std::cerr << "error: error was " << e.what() << "; expected " << expected_text << std::endl;
			throw;
		}
	}
}

int main() {
	test_expression("#xFF + #x80", "((+ 255) 128)");
	test_expression("(-)", "-");
	test_expression("'f + 2", "((+ (' f)) 2)");
	test_error_expression("()", "got <nothing>");
	test_error_expression("id ()", "got <nothing>");
	test_expression("[(map (\\i i) [])]", "((: ((map (\\i i)) [])) [])");
	test_expression("-2", "((- 0) 2)");
	test_expression("'a", "(' a)");
	//test_expression("'=", "(' =)"); // this conflicts with 'f + 2
	test_expression("'(=)", "(' =)");
	test_expression("(=)", "=");
	test_expression("b'a", "(b (' a))");
	test_expression("f g h", "((f g) h)");
	test_expression("f g h i", "(((f g) h) i)");
	test_expression("f g h i j", "((((f g) h) i) j)");
	test_expression("f g (2⋅h)", "((f g) ((* 2) h))");
	test_expression("2+3⋅5", "((+ 2) ((* 3) 5))");
	test_expression("2⋅3+5", "((+ ((* 2) 3)) 5)");
	test_expression("2⋅x+5", "((+ ((* 2) x)) 5)");
	test_expression("(2+3)⋅5", "((* ((+ 2) 3)) 5)");
	test_expression("(sin 2+3)⋅5", "((* ((+ (sin 2)) 3)) 5)");
	test_expression("(div 2 4 5+3)⋅5", "((* ((+ (((div 2) 4) 5)) 3)) 5)");
	test_expression("\\x x", "(\\x x)");
	test_expression("(\\x x) 1", "((\\x x) 1)");
	test_expression("(\\x x) (\\y y) 2", "(((\\x x) (\\y y)) 2)");
	test_expression("'a 'a", "((' a) (' a))");
	test_expression("a⨯b", "((⨯ a) b)");
	test_expression("a⃗⨯b⃗", "((⨯ a⃗) b⃗)");
	test_expression("cos cos x", "((cos cos) x)"); // well, it doesn't know that cos is a function.
	test_expression("f g\nh", "((f g) h)");
	test_expression("f\n  g h", "(f (g h))");
	test_expression("runIO\n  liftIO 2 ;\\v\n  liftIO 42", "(runIO ((; (liftIO 2)) (\\v (liftIO 42))))");
	test_expression("if (x = 0)\n  1\n$else\n  if (x = 2) 4\n  $else\n    2", "(($ ((if ((= x) 0)) 1)) (else (($ ((if ((= x) 2)) 4)) (else 2))))");
	test_expression("5 * \n  2 + 3", "((* 5) ((+ 2) 3))");
	test_expression("[]", "[]");
	test_expression("[1]", "((: 1) [])");
	test_expression("[1 2]", "((: 1) ((: 2) []))");
	test_expression("[1 '2]", "((: 1) ((: (' 2)) []))");
	test_expression("(\\f \\x 1)", "(\\f (\\x 1))");
	test_expression("(\\f \\x (f x)) (\\y y + 2) 1", "(((\\f (\\x (f x))) (\\y ((+ y) 2))) 1)");
	test_expression("(\\f \\x (f x) + 1) (\\y y + 2) 1", "(((\\f (\\x ((+ (f x)) 1))) (\\y ((+ y) 2))) 1)");
	test_expression("(\\f \\x (f x) + (f x)) (\\y y + 2) 1", "(((\\f (\\x ((+ (f x)) (f x)))) (\\y ((+ y) 2))) 1)");
	test_expression("let x = 2 in let y = 3 in x + y", "((\\x ((\\y ((+ x) y)) 3)) 2)");
	test_expression("a;b;c", "((; ((; a) b)) c)");
	test_expression("\\f \\list foldr (compose (:) f) nil list", "(\\f (\\list (((foldr ((compose :) f)) nil) list)))");

	//test_expression("2⋅f(x)", "((* 2) (f x))"); // doesn't work.
	return(0);
}
