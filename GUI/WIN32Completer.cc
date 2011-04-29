#include <malloc.h>
#include "GUI/WIN32Completer"

namespace GUI {

struct Completer {
	HWND entry;
	std::map<std::string, AST::Node*>* haystack;
};

/* note that haystack's values are unused. Only the keys are used and assumed to be AST::Symbol* */
void Completer_init(struct Completer* self, HWND entry, std::map<std::string, AST::Node*>* haystack) {
	self->entry = entry;
	self->haystack = haystack;
}
struct Completer* Completer_new(HWND entry, std::map<std::string, AST::Node*>* haystack) {
	struct Completer* result;
	result = (struct Completer*) calloc(1, sizeof(struct Completer));
	Completer_init(result,  entry, haystack);
	return(result);
}
void Completer_complete(struct Completer* self) {
}

}; /* end namespace */
