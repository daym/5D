#include "REPL/REPL2"
#include "REPL/Environment"

namespace REPL {

/* this file is unused */
/* dynamic scope */

struct REPL {
	Scanners::OperatorPrecedenceList* fOPL;
	Environment* fValenceEnvironment;
	AST::Node* fEnvironment;
	AST::Node* fEnvironmentTail;
};
OperatorPrecedenceList* getOperatorPrecedenceList(struct REPL* self) {
	return(self->fOPL);
}
void regenerateActualEnvironment(struct REPL* self) {
	/* fValenceEnvironment -> fEnvironment */
}
Environment* getEnvironment(struct REPL* self) {
	return(self->fValenceEnvironment);
}
AST::Node* describe(struct REPL* self, AST::Node* options, AST::Node* key) {
	/* TODO:
	- find the item in the environment, if any.
	- print the current value, or the one at backOffset specified in #options. */
	return(REPL_describe(self, key));
}
AST::Node* define(struct REPL* self, AST::Node* key, AST::Node* value) {
	/* TODO:
	- place the item in the environment, shifting existing nodes as necessary. 
	regenerateActualEnvironment() 
	*/
	return(REPL_define(self, key, value));
}
AST::Node* import(struct REPL* self, AST::Node* options, AST::Node* filename) {
	/* TODO:
	- mass-place the item in the environment.
	regenerateActualEnvironment() 
	*/
	return(REPL_import(self, filename));
}
void purge(struct REPL* self) {
	/* TODO:
	- traverse the hashtable, saving live items.
	*/
}
AST::Node* execute(struct REPL* self, AST::Node* node) {
	/* TODO:
	- eval the node in the environment fEnvironment.
	- if possible, avoid having to copy over all of the fEnvironment nodes every time (possibly by having a copy where the actual environment lives in).
	*/
}

}; /* end namespace REPL */
