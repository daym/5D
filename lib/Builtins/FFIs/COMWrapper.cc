#include "Values/Values"
#include "Evaluators/FFI"
#include "FFIs/COMWrapper"

namespace FFIs {
using namespace Values;

// TODO make that nicer...

IDispatch* unwrapIDispatch(NodeT source) {
	return (IDispatch*) Evaluators::get_pointer(source);
}
IUnknown* unwrapIUnknown(NodeT source) {
	return (IUnknown*) Evaluators::get_pointer(source);
}
NodeT wrapIDispatch(IDispatch* source) {
	return(makeBox(source, NULL)); // TODO make that nicer.
}
NodeT wrapIUnknown(IUnknown* source) {
	return(makeBox(source, NULL));
}

}; /* end namespace FFIs */

