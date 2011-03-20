#include <assert.h>
#include "Evaluators/Evaluators"
#include "AST/AST"

namespace Evaluators {
using namespace AST;

void get_free_variables(AST::Node* root, std::set<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& freeNames) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	if(consNode) {
		AST::Node* headNode = consNode->head;
		if(headNode == intern("\\") && consNode->tail) { // abstraction.
			AST::Node* parameterNode = consNode->tail->head;
			AST::Symbol* parameterSymbolNode = dynamic_cast<AST::Symbol*>(parameterNode);
			assert(parameterSymbolNode);
			if(boundNames.find(symbolNode) == boundNames.end()) { // not bound yet
				boundNames.insert(parameterSymbolNode);
				get_free_variables(consNode->tail->tail, boundNames, freeNames);
				boundNames.erase(parameterSymbolNode);
			} else // already bound to something else: make sure not to get rid of it.
				get_free_variables(consNode->tail->tail, boundNames, freeNames);
		} else { // application etc.
		}
	} else if(symbolNode) {
		if(boundNames.find(symbolNode) == boundNames.end()) // not bound is free.
			freeNames.insert(symbolNode);
	} // else other stuff.
}

}; // end namespace Evaluators.
