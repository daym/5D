#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <vector>
#include <deque>
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "AST/AST"
#include "Scanners/MathParser"
#include "AST/Symbols"

namespace GUI {
bool interrupted_P(void);
};

namespace Evaluators {
using namespace AST;

EvaluationException::EvaluationException(const char* s) throw() {
	message = strdup(s);
}
const char* EvaluationException::what() const throw() {
	return message; //message.c_str();
};

static void get_free_variables_impl(AST::Node* root, std::set<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& freeNames) {
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	if(abstraction_P(root)) {
		AST::Node* parameterNode = get_abstraction_parameter(root);
		AST::Symbol* parameterSymbolNode = dynamic_cast<AST::Symbol*>(parameterNode);
		assert(parameterSymbolNode);
		AST::Node* body = get_abstraction_body(root);
		if(boundNames.find(symbolNode) == boundNames.end()) { // not bound yet
			boundNames.insert(parameterSymbolNode);
			get_free_variables_impl(body, boundNames, freeNames);
			boundNames.erase(parameterSymbolNode);
		} else // already bound to something else: make sure not to get rid of it.
			get_free_variables_impl(body, boundNames, freeNames);
	} else if(application_P(root)) {
		get_free_variables_impl(get_application_operator(root), boundNames, freeNames);
		get_free_variables_impl(get_application_operand(root), boundNames, freeNames);
	} else if(symbolNode) {
		if(boundNames.find(symbolNode) == boundNames.end()) // not bound is free.
			freeNames.insert(symbolNode);
	} // else other stuff.
}
void get_free_variables(AST::Node* root, std::set<AST::Symbol*>& freeNames) {
	std::set<AST::Symbol*> boundNames;
	get_free_variables_impl(root, boundNames, freeNames);
}
static AST::Symbol* get_variable_name(AST::Node* root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->symbol);
	else
		return(NULL);
}
static inline int get_variable_index(AST::Node* root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->index);
	else
		return(-1);
}
static bool quote_P(AST::Node* root) {
	if(root == Symbols::Squote)
		return(true);
	else {
		AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(root);
		return(ref && ref->symbol == Symbols::Squote);
	}
}
AST::Node* annotate_impl(AST::Node* root, std::deque<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& boundNamesSet) {
	// TODO maybe traverse cons etc? maybe not.
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	AST::Node* result;
	if(abstraction_P(root)) {
		AST::Node* parameterNode = get_abstraction_parameter(root);
		AST::Node* body = get_abstraction_body(root);
		AST::Symbol* parameterSymbolNode = dynamic_cast<AST::Symbol*>(parameterNode);
		assert(parameterSymbolNode);
		boundNames.push_front(parameterSymbolNode);
		if(boundNamesSet.find(symbolNode) == boundNamesSet.end()) { // not bound yet
			boundNamesSet.insert(parameterSymbolNode);
			result = annotate_impl(body, boundNames, boundNamesSet);
			boundNamesSet.erase(parameterSymbolNode);
		} else // already bound to something else: make sure not to get rid of it.
			result = annotate_impl(body, boundNames, boundNamesSet);
		assert(!boundNames.empty() && boundNames.front() == parameterSymbolNode);
		boundNames.pop_front();
		if(result == body) // reuse the original nodes if we didn't migrate off them.
			return(root);
		else
			return(makeAbstraction(parameterNode, result));
	} else if(application_P(root)) {
		AST::Node* operator_ = get_application_operator(root);
		AST::Node* operand = get_application_operand(root);
		AST::Node* newOperatorNode = annotate_impl(operator_, boundNames, boundNamesSet);
		AST::Node* newOperandNode = quote_P(newOperatorNode) ? operand : annotate_impl(operand, boundNames, boundNamesSet);
		if(operator_ == newOperatorNode && operand == newOperandNode)
			return(root);
		else
			return(makeApplication(newOperatorNode, newOperandNode));
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
			return(makeApplication(new_fn, new_argument));
	} else if(abstraction_P(term)) {
		AST::Node* body;
		AST::Node* parameter;
		AST::Node* new_body;
		body = get_abstraction_body(term);
		if(body) {
			parameter = get_abstraction_parameter(term);
			new_body = shift(argument, index + 1, body);
		} else
			new_body = NULL;
		if(body == new_body)
			return(term);
		else
			return(makeAbstraction(parameter, new_body));
	} else 
		return(term);
}
AST::Node* Reducer::execute(AST::Node* argument) {
	return(argument);
}
REGISTER_STR(Reducer, return("simplify");)
Reducer reducer;
static bool wants_its_argument_reduced_P(AST::Node* fn) {
	Operation* fnOperation = dynamic_cast<Operation*>(fn);
	return(fnOperation ? fnOperation->eager_P() : fn == &reducer);
}
static int recursionLevel = 0; /* anti-endless-loop */

// caching results.
static int fGeneration = 1;
static bool application_result_P(AST::Node* app) {
	return(((AST::Application*) app)->resultGeneration == fGeneration);
}
static AST::Node* remember(AST::Node* app, AST::Node* result) {
	((AST::Application*) app)->result = result;
	((AST::Application*) app)->resultGeneration = fGeneration;
	return(result);
}
AST::Node* get_application_result(AST::Node* n) {
	AST::Application* app = (AST::Application*) n;
	if(app->resultGeneration != fGeneration) {
		app->result = NULL;
		reduce(app);
	}
	return(app->result);
}
int increaseGeneration(void) {
	++fGeneration;
	if(fGeneration < 0) // sigh
		fGeneration = 0;
	return(fGeneration);
}
static AST::Node* ensureApplication(AST::Node* term, AST::Node* fn, AST::Node* argument) {
	if(get_application_operator(term) == fn && get_application_operand(term) == argument)
		return(term);
	else
		return(makeApplication(fn, argument));
}
AST::Node* reduce(AST::Node* term) {
	if(GUI::interrupted_P())
		throw EvaluationException("evaluation was interrupted");
	if(recursionLevel > 1000) {
		recursionLevel = 0;
		throw EvaluationException("recursion was too deep");
	}
	if(application_P(term)) {
		if(application_result_P(term))
			return(((AST::Application*) term)->result);
		AST::Node* fn;
		AST::Node* argument;
		++recursionLevel;
		fn = reduce(get_application_operator(term));
		--recursionLevel;
		argument = get_application_operand(term);
		if(wants_its_argument_reduced_P(fn)) {
			++recursionLevel;
			argument = reduce(argument);
			--recursionLevel;
		}
		if(abstraction_P(fn)) {
			AST::Node* body;
			body = get_abstraction_body(fn);
			body = shift(argument, 1, body);
			++recursionLevel;
			body = reduce(body);
			--recursionLevel;
			return(remember(term, body));
		} else {
			// most of the time, SymbolReference anyway: AST::Symbol* fnName = dynamic_cast<AST::Symbol*>(fn);
			Evaluators::Operation* fnOperation = dynamic_cast<Evaluators::Operation*>(fn);
			if(fnOperation/* && !application_P(argument)*/) {
				AST::Node* result;
				result = fnOperation->execute(argument);
				return(remember(term, result));
			} else {
				// TODO: remove this:
				// BuiltinOperation are almost unreadable and they can't resolve it anyway, so why bother with it?
				// note that this could be moved *into* BuiltinOperation, although I don't see any of them reacting any other way than this:
				AST::BuiltinOperation* bnOperation = NULL;
				for(; (bnOperation = dynamic_cast<AST::BuiltinOperation*>(fn)) != NULL; fn = bnOperation->fallback) {
				}
				return(remember(term, ensureApplication(term, fn, argument)));
			}
		}
	} else if(abstraction_P(term)) {
		/* this isn't strictly necessary, but nicer */
		AST::Node* body;
		AST::Node* parameter;
		AST::Node* new_body;
		new_body = body = get_abstraction_body(term);
		parameter = get_abstraction_parameter(term);
#if 0
		try {
			new_body = reduce(body);
		} catch (Evaluators::EvaluationException e) {
			/* recursion too deep etc */
			/* FIXME this is WAY slow, so remove it again? */
			new_body = body;
		}
#endif
		if(new_body == body)
			return(term);
		else
			return(makeAbstraction(parameter, new_body));
	} else
		return(term);
}
AST::Node* close(AST::Symbol* parameter, AST::Node* argument, AST::Node* body) {
	return(makeApplication(makeAbstraction(parameter, body), argument));
}
AST::Node* makeError(const char* reason) {
	// FIXME!!
	return(AST::makeStr(reason));
}
bool define_P(AST::Node* input) {
	return(input != NULL && application_P(input) && get_application_operator(input) == Symbols::Sdefine);
}
AST::Node* evaluate(AST::Node* computation) {
	if(application_P(computation))
		return(get_application_result(computation));
	else
		return(computation);
}
AST::Cons* evaluateToCons(AST::Node* computation) {
	return(dynamic_cast<AST::Cons*>(evaluate(computation))); // TODO error check
}
AST::Node* programFromSExpression(AST::Node* root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	if(consNode) {
		// application or abstraction
		if(consNode->head == Symbols::Sbackslash) { // abstraction
			assert(consNode->tail);
			assert(evaluateToCons(consNode->tail)->tail);
			assert(evaluateToCons(evaluateToCons(consNode->tail)->tail)->tail == NULL);
			AST::Node* parameter = evaluateToCons(consNode->tail)->head;
			AST::Node* body = evaluateToCons(evaluateToCons(consNode->tail)->tail)->head;
			return(makeAbstraction(programFromSExpression(parameter), programFromSExpression(body)));
		} else { // application
			assert(evaluateToCons(consNode->tail));
			assert(evaluateToCons(evaluateToCons(consNode->tail)->tail) == NULL);
			AST::Node* operator_ = consNode->head;
			AST::Node* operand = evaluateToCons(consNode->tail)->head;
			return(makeApplication(programFromSExpression(operator_), programFromSExpression(operand)));
		}
	} else
		return(root);
}

AST::Node* listFromCharZ(const char* text) {
	if(*text == 0)
		return(NULL);
	else
		return(makeCons(Numbers::internNative((Numbers::NativeInt) (unsigned char) *text), listFromCharZ(text + 1)));
}
AST::Node* listFromStr(AST::Str* node) {
	return(listFromCharZ(node->text.c_str()));
}
AST::Node* strFromList(AST::Cons* node) {
	std::stringstream sst;
	bool B_ok;
	for(; node; node = evaluateToCons(node->tail)) {
		int c = Numbers::toNativeInt(node->head, B_ok);
		if(c < 0 || c > 255) // oops
			return(makeApplication(Symbols::SstrFromList, node)); 
		sst << (char) c;
	}
	return(makeStr(sst.str().c_str()));
}


}; // end namespace Evaluators.
