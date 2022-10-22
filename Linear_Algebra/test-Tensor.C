#include <stdlib.h>
#include "Tensor"

int main() {
	using namespace Linear_Algebra;
	Tensor<int, 4, 1> a;
	a.items[1] = 2;
	return(0);
}
