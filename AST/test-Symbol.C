#include <stdlib.h>
#include "AST/Symbol"

int main() {
	using namespace AST;
	Symbol* hello = intern("hello");
	Symbol* hello2 = intern("hello");
	if(hello != hello2)
		abort();
	return(0);
}
