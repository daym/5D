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
#include <5D/Values>
#include "Values/Symbol"

static void gc_test(void) {
	int i;
	for(i = 0; i < 1000; ++i)
		GC_MALLOC(1000);
}
int main() {
	using namespace Values;
	NodeT hello = symbolFromStr("hello");
	NodeT hello2 = symbolFromStr("hello");
	gc_test();
	GC_gcollect();
	assert(symbol_P(hello));
	assert(symbol_P(hello2));
	assert(strcmp(get_symbol_name(hello), get_symbol_name(hello2)) == 0);
	if(hello != hello2)
		abort();
	return(0);
}
