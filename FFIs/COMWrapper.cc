#include "Values/Values"
#include "Evaluators/FFI"
#include "FFIs/COMWrapper"

namespace FFIs {

// TODO make that nicer...

IDispatch* unwrapIDispatch(AST::NodeT source) {
	return (IDispatch*) Evaluators::get_pointer(source);
}
IUnknown* unwrapIUnknown(AST::NodeT source) {
	return (IUnknown*) Evaluators::get_pointer(source);
}
AST::NodeT wrapIDispatch(IDispatch* source) {
	return(AST::makeBox(source, NULL)); // TODO make that nicer.
}
AST::NodeT wrapIUnknown(IUnknown* source) {
	return(AST::makeBox(source, NULL));
}

}; /* end namespace FFIs */

