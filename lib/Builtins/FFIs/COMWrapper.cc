#include "Values/Values"
#include "Evaluators/FFI"
#include "FFIs/COMWrapper"
#include <5D/FFIs>

namespace FFIs {
using namespace Values;

// TODO make that nicer...

IDispatch* unwrapIDispatch(NodeT source) {
	return (IDispatch*) Values::pointerFromNode(source);
}
IUnknown* unwrapIUnknown(NodeT source) {
	return (IUnknown*) Values::pointerFromNode(source);
}
NodeT wrapIDispatch(IDispatch* source) {
	return(makeBox(source, NULL)); // TODO make that nicer.
}
NodeT wrapIUnknown(IUnknown* source) {
	return(makeBox(source, NULL));
}

}; /* end namespace FFIs */

