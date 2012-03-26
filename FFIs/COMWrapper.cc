#include "AST/AST"
#include "Evaluators/FFI"
#include "FFIs/COMWrapper"

namespace FFIs {

// TODO make that nicer...

IDispatch* unwrapIDispatch(AST::Node* source) {
	return (IDispatch*) Evaluators::get_pointer(source);
}
IUnknown* unwrapIUnknown(AST::Node* source) {
	return (IUnknown*) Evaluators::get_pointer(source);
}
AST::Node* wrapIDispatch(IDispatch* source) {
	return(AST::makeBox(source, NULL)); // TODO make that nicer.
}
AST::Node* wrapIUnknown(IUnknown* source) {
	return(AST::makeBox(source, NULL));
}

}; /* end namespace FFIs */

