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
