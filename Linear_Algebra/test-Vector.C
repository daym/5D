#include <stdlib.h>
#include "Vector"

int main() {
	using namespace Linear_Algebra;
	Vector<int> v;
	// test element access
	v.items[0] = 1;
	v.items[1] = 2;
	v.items[2] = 30000;
	v.items[3] = 400000;
	if(v.z != 400000 || v.y != 30000 || v.x != 2 || v.ct != 1)
		abort();
	// test copy constructor
	Vector<int> v2 = v;
	if(v2.z != 400000 || v2.y != 30000 || v2.x != 2 || v2.ct != 1)
		abort();
	// test assignment
	Vector<int> v3;
	v3.z = 1;
	v3 = v;
	if(v3.z != 400000 || v3.y != 30000 || v3.x != 2 || v3.ct != 1)
		abort();
	// test addition
	v3 = v + v;
	if(v3.z != 400000*2 || v3.y != 30000*2 || v3.x != 2*2 || v3.ct != 1*2)
		abort();
	// test subtraction
	v3 = v - v;
	if(v3.z != 0 || v3.y != 0 || v3.x != 0 || v3.ct != 0)
		abort();
	// test negation
	v3 = -v;
	if(v3.z != -400000 || v3.y != -30000 || v3.x != -2 || v3.ct != -1)
		abort();
	return(0);
}
