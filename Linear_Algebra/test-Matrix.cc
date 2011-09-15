/*
5D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include "Matrix"

int main() {
	using namespace Linear_Algebra;
	Matrix<int,4,1> v;
	// test element access
	v.items[0][0] = 1;
	v.items[1][0] = 2;
	v.items[2][0] = 30000;
	v.items[3][0] = 400000;
	if(v.items[3][0] != 400000 || v.items[2][0] != 30000 || v.items[1][0] != 2 || v.items[0][0] != 1)
		abort();
	// test copy constructor
	Matrix<int,4,1> v2 = v;
	if(v2.items[3][0] != 400000 || v2.items[2][0] != 30000 || v2.items[1][0] != 2 || v2.items[0][0] != 1)
		abort();
	// test assignment
	Matrix<int,4,1> v3;
	v3.items[3][0] = 1;
	v3 = v;
	if(v3.items[3][0] != 400000 || v3.items[2][0] != 30000 || v3.items[1][0] != 2 || v3.items[0][0] != 1)
		abort();
	// test addition
	v3 = v + v;
	if(v3.items[3][0] != 400000*2 || v3.items[2][0] != 30000*2 || v3.items[1][0] != 2*2 || v3.items[0][0] != 1*2)
		abort();
	// test subtraction
	v3 = v - v;
	if(v3.items[3][0] != 0 || v3.items[2][0] != 0 || v3.items[1][0] != 0 || v3.items[0][0] != 0)
		abort();
	// test negation
	v3 = -v;
	if(v3.items[3][0] != -400000 || v3.items[2][0] != -30000 || v3.items[1][0] != -2 || v3.items[0][0] != -1)
		abort();
	return(0);
}
