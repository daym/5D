#include <assert.h>
#include <string.h>
#include <vector>
#include <deque>
#include "Evaluators/Evaluators"
#include "AST/AST"

namespace Evaluators {
using namespace AST;

EvaluationException::EvaluationException(const char* s) throw() {
	message = strdup(s);
}
const char* EvaluationException::what() const throw() {
	return message; //message.c_str();
};


static void get_free_variables_impl(AST::Node* root, std::set<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& freeNames) {
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
				get_free_variables_impl(consNode->tail->tail, boundNames, freeNames);
				boundNames.erase(parameterSymbolNode);
			} else // already bound to something else: make sure not to get rid of it.
				get_free_variables_impl(consNode->tail->tail, boundNames, freeNames);
		} else { // application etc.
			get_free_variables_impl(consNode->tail, boundNames, freeNames);
		}
	} else if(symbolNode) {
		if(boundNames.find(symbolNode) == boundNames.end()) // not bound is free.
			freeNames.insert(symbolNode);
	} // else other stuff.
}
void get_free_variables(AST::Node* root, std::set<AST::Symbol*>& freeNames) {
	std::set<AST::Symbol*> boundNames;
	get_free_variables_impl(root, boundNames, freeNames);
}
AST::Node* annotate_impl(AST::Node* root, std::deque<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& boundNamesSet) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	AST::Node* result;
	if(consNode) {
		AST::Node* headNode = consNode->head;
		if(headNode == intern("\\") && consNode->tail) { // abstraction.
			AST::Node* parameterNode = consNode->tail->head;
			AST::Symbol* parameterSymbolNode = dynamic_cast<AST::Symbol*>(parameterNode);
			assert(parameterSymbolNode);
			boundNames.push_front(parameterSymbolNode);
			if(boundNamesSet.find(symbolNode) == boundNamesSet.end()) { // not bound yet
				boundNamesSet.insert(parameterSymbolNode);
				result = annotate_impl(consNode->tail->tail, boundNames, boundNamesSet);
				boundNamesSet.erase(parameterSymbolNode);
			} else // already bound to something else: make sure not to get rid of it.
				result = annotate_impl(consNode->tail->tail, boundNames, boundNamesSet);
			assert(!boundNames.empty() && boundNames.front() == parameterSymbolNode);
			boundNames.pop_front();
		} else { // application etc.
			// headNode
			AST::Node* newHeadNode = annotate_impl(headNode, boundNames, boundNamesSet);
			AST::Node* newTailNode = annotate_impl(consNode->tail, boundNames, boundNamesSet);
			AST::Cons* newTailCons = dynamic_cast<AST::Cons*>(newTailNode);
			if(newHeadNode == headNode && newTailNode == consNode->tail)
				return(consNode);
			if(!newTailCons && newTailNode) /* not a cons. */
				throw EvaluationException("cons tail is not a cons");
			return(cons(newHeadNode, newTailCons));
		}
		return(result);
	} else if(symbolNode) {
		int size = boundNames.size();
		int i;
		for(i = 0; i < size; ++i)
			if(boundNames[i] == symbolNode)
				break;
		if(i < size) { /* found */
			//std::distance(boundNames.begin(), std::find(boundNames.begin(), boundNames.end(), symbolNode));
			SymbolReference* ref = new SymbolReference();
			ref->symbol = symbolNode;
			ref->index = i + 1;
			return(ref);
		}
	} // else other stuff.
	return(root);
}
AST::Node* annotate(AST::Node* root) {
	std::deque<AST::Symbol*> boundNames;
	std::set<AST::Symbol*> boundNamesSet;
	return(annotate_impl(root, boundNames, boundNamesSet));
}

}; // end namespace Evaluators.
