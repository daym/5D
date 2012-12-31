/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "Values/Values"
#include "AST/Symbol"

static void gc_test(void) {
	int i;
	for(i = 0; i < 1000; ++i)
		GC_MALLOC(1000);
}
int main() {
	using namespace AST;
	Symbol* hello = symbolFromStr("hello");
	Symbol* hello2 = symbolFromStr("hello");
	gc_test();
	GC_gcollect();
	assert(dynamic_cast<Symbol*>(hello) != NULL);
	assert(dynamic_cast<Symbol*>(hello2) != NULL);
	assert(strcmp(hello->name, hello2->name) == 0);
	if(hello != hello2)
		abort();
	return(0);
}
