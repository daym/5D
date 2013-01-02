#include <string.h>
#include <limits>
#include "Numbers/Real"
#include "Evaluators/Builtins"

namespace Evaluators {
using namespace Values;
NodeT internNative(bool value);
};

namespace Numbers {
using namespace Values;
REGISTER_STR(Float, {
	std::stringstream sst;
	sst.precision(std::numeric_limits<NativeFloat>::digits10 + 1);
	sst << node->value;
	std::string v = sst.str();
	const char* vc = v.c_str();
	if(*vc == '-')
		++vc;
	if(strpbrk(vc, ".eE") == NULL && (vc[0] >= '0' && vc[0] <= '9')) { /* not a special value like inf or nan */
		sst << ".0";
		v = sst.str();
	}
	return(v);
})

REGISTER_STR(Real, return("FIXME");)

NodeT internNative(NativeFloat value) {
	return(new Float(value));
}

bool float_P(NodeT node) {
	return(dynamic_cast<Float*>(node) != NULL);
}
DEFINE_SIMPLE_STRICT_OPERATION(FloatP, float_P(argument))

NodeT operator+(const Float& a, const Float& b) {
	return(internNative(a.value + b.value)); /* FIXME */
}
NodeT operator-(const Float& a, const Float& b) {
	return(internNative(a.value - b.value)); /* FIXME */
}
NodeT operator*(const Float& a, const Float& b) {
	return(internNative(a.value * b.value)); /* FIXME */
}
NodeT operator/(const Float& a, const Float& b) {
	return(internNative(a.value / b.value)); /* FIXME */
}
Real* operator+(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator-(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator*(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
Real* operator/(const Real& a, const Real& b) {
	return(NULL); /* FIXME */
}
NodeT operator<=(const Float& a, const Float& b) {
	return(Evaluators::internNative(a.value <= b.value));
}
NodeT operator<=(const Real& a, const Real& b) {
	return(Evaluators::internNative(false)); /* FIXME */
}

REGISTER_BUILTIN(FloatP, 1, 0, symbolFromStr("float?"))

bool toNativeFloat(NodeT node, NativeFloat& result) {
	Float* floatNode;
	Real* realNode;
	result = 0.0;
	//node = evaluate(node);
	if(node == NULL)
		return(false);
	else if((floatNode = dynamic_cast<Float*>(node)) != NULL) {
		result = floatNode->value;
		return(true);
	} else if((realNode = dynamic_cast<Real*>(node)) != NULL) {
		//NativeFloat result = realNode->toNativeFloat();
		return(false); // FIXME
	} else {
		// only coerce integers to float if there is no information loss
		NativeInt value = 0;
		if(!toNativeInt(node, value))
			return(false);
		result = (NativeFloat) value;
		return((NativeInt) result == value);
	}
}

#ifdef __GNUC__
#define LLVM_INF           __builtin_inff()
static Float nanFloat(__builtin_nanf(""));
static Float infinityFloat(__builtin_inff());
#else
static Float nanFloat(0.0/0.0);
static Float infinityFloat(1.0/0.0);
#endif

NodeT nan(void) {
	return(&nanFloat);
}
NodeT infinity(void) {
	return(&infinityFloat);
}

}; /* end namespace Numbers */
