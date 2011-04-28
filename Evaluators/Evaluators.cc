#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <deque>
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Scanners/MathParser"

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
			get_free_variables_impl(consNode->head, boundNames, freeNames);
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
static AST::Node* get_abstraction_body(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	if(consNode && consNode->head == intern("\\") && consNode->tail && consNode->tail->tail) {
		/* take the trailing item as body */
		return(follow_tail(consNode->tail->tail)->head);
	} else
		return(NULL);
}
bool abstraction_P(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	if(!consNode)
		return(false);
	else
		return(consNode->head == intern("\\") && consNode->tail);
}
bool application_P(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	if(!consNode)
		return(false);
	else
		return(consNode->head != intern("\\"));
}
static AST::Node* get_application_operator(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	return(consNode ? consNode->head : NULL);
}
static AST::Node* get_application_operand(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	return((consNode && consNode->tail) ? consNode->tail->head : NULL);
}
static AST::Symbol* get_variable_name(AST::Node* root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->symbol);
	else
		return(NULL);
}
static int get_variable_index(AST::Node* root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->index);
	else
		return(-1);
}
static bool quote_P(AST::Node* root) {
	if(root == AST::intern("quote"))
		return(true);
	else {
		AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(root);
		return(ref && ref->symbol == AST::intern("quote"));
	}
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
			if(result == consNode->tail->tail) // reuse the original nodes if we didn't migrate off them.
				result = consNode;
			else
				result = cons(headNode, cons(parameterNode, dynamic_cast<AST::Cons*>(result)));
		} else { // application etc.
			// headNode
			AST::Node* newHeadNode = annotate_impl(headNode, boundNames, boundNamesSet);
			//AST::Node* newTailNode = annotate_impl(consNode->tail, boundNames, boundNamesSet);
			AST::Node* newTailNode = quote_P(newHeadNode) ? consNode->tail : annotate_impl(consNode->tail, boundNames, boundNamesSet);
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
			SymbolReference* ref = new SymbolReference(symbolNode, i + 1);
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
static AST::Node* shift(AST::Node* argument, int index, AST::Node* term) {
	int x_index;
	x_index = get_variable_index(term);
	if(x_index != -1) {
		if(x_index == index)
			return(argument);
		else if(x_index > index)
			return(new SymbolReference(get_variable_name(term), x_index - 1));
		else
			return(term);
	} else if(application_P(term)) {
		AST::Node* x_fn;
		AST::Node* x_argument;
		AST::Node* new_fn;
		AST::Node* new_argument;
		x_fn = get_application_operator(term);
		x_argument = get_application_operand(term);
		new_fn = shift(argument, index, x_fn);
		new_argument = shift(argument, index, x_argument);
		if(new_fn == x_fn && new_argument == x_argument)
			return(term);
		else
			return(cons(new_fn, cons(new_argument, NULL)));
	} else if(abstraction_P(term)) {
		AST::Node* body;
		AST::Node* parameter;
		AST::Node* new_body;
		body = get_abstraction_body(term);
		if(body) {
			parameter = dynamic_cast<AST::Cons*>(term)->tail->head;
			new_body = shift(argument, index + 1, body);
		} else
			new_body = NULL;
		if(body == new_body)
			return(term);
		else
			return(cons(intern("\\"), cons(parameter, cons(new_body, NULL))));
	} else 
		return(term);
}
static bool wants_its_argument_reduced_P(AST::Node* fn) {
	Operation* fnOperation = dynamic_cast<Operation*>(fn);
	return(fnOperation ? fnOperation->eager_P() : true);
}
AST::Node* reduce(AST::Node* term) {
	if(application_P(term)) {
		AST::Node* fn;
		AST::Node* argument;
		fn = reduce(get_application_operator(term));
		argument = get_application_operand(term);
		if(wants_its_argument_reduced_P(fn))
			argument = reduce(argument);
		if(abstraction_P(fn)) {
			AST::Node* body;
			body = get_abstraction_body(fn);
			body = shift(argument, 1, body);
			return(reduce(body));
		} else {
			// most of the time, SymbolReference anyway: AST::Symbol* fnName = dynamic_cast<AST::Symbol*>(fn);
			Evaluators::Operation* fnOperation = dynamic_cast<Evaluators::Operation*>(fn);
			if(fnOperation) {
				return(fnOperation->execute(argument));
			} else if(get_application_operator(term) == fn && get_application_operand(term) == argument)
				return(term);
			else
				return(AST::cons(fn, AST::cons(argument, NULL)));
		}
	} else if(abstraction_P(term)) {
		return(term);
	} else
		return(term);
}

// Operation
bool Operation::eager_P(void) const {
	return(true);
}
AST::Node* close(AST::Symbol* parameter, AST::Node* argument, AST::Node* body) {
	return(cons(cons(AST::intern("\\"), cons(parameter, cons(body, NULL))), cons(argument, NULL)));
}

}; // end namespace Evaluators.
