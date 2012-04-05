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
#include "Evaluators/Operation"
#include "FFIs/Allocators"

namespace GUI {
bool interrupted_P(void);
};

namespace Evaluators {
using namespace AST;

EvaluationException::EvaluationException(const char* s) throw() {
	message = GCx_strdup(s);
}
const char* EvaluationException::what() const throw() {
	return message; //message.c_str();
};

static void get_free_variables_impl(AST::NodeT root, std::set<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& freeNames) {
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	if(abstraction_P(root)) {
		AST::NodeT parameterNode = get_abstraction_parameter(root);
		AST::Symbol* parameterSymbolNode = dynamic_cast<AST::Symbol*>(parameterNode);
		assert(parameterSymbolNode);
		AST::NodeT body = get_abstraction_body(root);
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
void get_free_variables(AST::NodeT root, std::set<AST::Symbol*>& freeNames) {
	std::set<AST::Symbol*> boundNames;
	get_free_variables_impl(root, boundNames, freeNames);
}
static AST::Symbol* get_variable_name(AST::NodeT root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->symbol);
	else
		return(NULL);
}
static inline int get_variable_index(AST::NodeT root) {
	AST::SymbolReference* refNode = dynamic_cast<AST::SymbolReference*>(root);
	if(refNode)
		return(refNode->index);
	else
		return(-1);
}
bool quote_P(AST::NodeT root) {
	if(root == Symbols::Squote || root == &Quoter)
		return(true);
	else {
		AST::SymbolReference* ref = dynamic_cast<AST::SymbolReference*>(root);
		return(ref && ref->symbol == Symbols::Squote);
	}
}
static inline bool quoted_P(AST::NodeT root) {
	return(quote_P(root));
}
AST::NodeT annotate_impl(AST::NodeT root, std::deque<AST::Symbol*>& boundNames, std::set<AST::Symbol*>& boundNamesSet) {
	// TODO maybe traverse cons etc? maybe not.
	AST::Symbol* symbolNode = dynamic_cast<AST::Symbol*>(root);
	AST::NodeT result;
	if(abstraction_P(root)) {
		AST::NodeT parameterNode = get_abstraction_parameter(root);
		AST::NodeT body = get_abstraction_body(root);
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
		AST::NodeT operator_ = get_application_operator(root);
		AST::NodeT operand = get_application_operand(root);
		if(operator_ == &Reducer || operator_ == Symbols::Sinline) { // ideally this would be auto-detected, but it isn't right now.
			return(annotate_impl(reduce1(operand), boundNames, boundNamesSet));
		}
		AST::NodeT newOperatorNode = annotate_impl(operator_, boundNames, boundNamesSet);
		AST::NodeT newOperandNode = quoted_P(newOperatorNode) ? operand : annotate_impl(operand, boundNames, boundNamesSet);
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
		} else {
			std::stringstream sst;
			sst << "(" << str(symbolNode) << ") is not bound";
			std::string v = sst.str();
			throw EvaluationException(v.c_str()); // TODO line info...
		}
	} // else other stuff.
	return(root);
}
AST::NodeT annotate(AST::NodeT root) {
	std::deque<AST::Symbol*> boundNames;
	std::set<AST::Symbol*> boundNamesSet;
	return(annotate_impl(root, boundNames, boundNamesSet));
}
static AST::NodeT shift(AST::NodeT argument, int index, AST::NodeT term) {
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
		AST::NodeT x_fn;
		AST::NodeT x_argument;
		AST::NodeT new_fn;
		AST::NodeT new_argument;
		x_fn = get_application_operator(term);
		x_argument = get_application_operand(term);
		new_fn = shift(argument, index, x_fn);
		new_argument = shift(argument, index, x_argument);
		if(new_fn == x_fn && new_argument == x_argument)
			return(term);
		else
			return(makeApplication(new_fn, new_argument));
	} else if(abstraction_P(term)) {
		AST::NodeT body;
		AST::NodeT parameter;
		AST::NodeT new_body;
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
DEFINE_SIMPLE_OPERATION(Reducer, reduce(argument))
DEFINE_SIMPLE_OPERATION(Quoter, argument)
static inline bool wants_its_argument_reduced_P(AST::NodeT fn) {
	return(false); /* FIXME check for reducer, quoter etc */
}
/* TODO static */
int recursionLevel = 0; /* anti-endless-loop */

// caching results.
int fGeneration = 1;
static AST::NodeT remember(AST::NodeT app, AST::NodeT result) {
	((AST::Application*) app)->result = result;
	((AST::Application*) app)->resultGeneration = fGeneration;
	return(result);
}
AST::NodeT get_application_result(AST::NodeT n) {
	AST::Application* app = (AST::Application*) n;
	if(app->resultGeneration != fGeneration) {
		app->result = NULL;
		reduce1(app);
	}
	return(app->result);
}
int increaseGeneration(void) {
	++fGeneration;
	if(fGeneration < 0) // sigh
		fGeneration = 0;
	return(fGeneration);
}
static AST::NodeT ensureApplication(AST::NodeT term, AST::NodeT fn, AST::NodeT argument) {
	if(get_application_operator(term) == fn && get_application_operand(term) == argument)
		return(term);
	else
		return(makeApplication(fn, argument));
}
typedef AST::NodeT (replace_predicate_t)(void* userData, AST::NodeT node);
AST::NodeT mapTree(void* userData, replace_predicate_t* replacer, AST::NodeT term) {/* intended for builtins only */
	// TODO CurriedOperation ? 
	if(term)
		term = replacer(userData, term);
	if(term == NULL) 
		return(NULL);
	else if(application_P(term)) {
		AST::NodeT fn = get_application_operand(term);
		AST::NodeT argument = get_application_operator(term);
		AST::NodeT new_fn;
		AST::NodeT new_argument;
		new_fn = mapTree(userData, replacer, fn);
		new_argument = mapTree(userData, replacer, argument);
		if(new_fn == fn && new_argument == argument)
			return(term);
		else
			return(makeApplication(fn, argument));
	} else if(abstraction_P(term)) {
		AST::NodeT body;
		AST::NodeT parameter;
		AST::NodeT new_body;
		AST::NodeT new_parameter;
		new_body = body = get_abstraction_body(term);
		parameter = get_abstraction_parameter(term);
		new_body = mapTree(userData, replacer, body);
		new_parameter = mapTree(userData, replacer, parameter);
		if(new_body == body && new_parameter == parameter)
			return(term);
		else
			return(makeAbstraction(parameter, new_body));
	} else if(curried_operation_P(term)) {
		AST::NodeT operator_ = Evaluators::get_curried_operation_operation(term);
		AST::NodeT operand = Evaluators::get_curried_operation_argument(term);
		AST::NodeT new_operator_ = mapTree(userData, replacer, operator_);
		AST::NodeT new_operand = mapTree(userData, replacer, operand);
		if(new_operator_ == operator_ && new_operand == operand)
			return(term);
		else
			return(makeCurriedOperation(new_operator_, new_operand));
	} else
		return(term);
}
static AST::NodeT replaceSingleNode(void* userData, AST::NodeT node) {
	AST::Cons* data = (AST::Cons*) userData;
	if(node == data->head) {
		return(Evaluators::evaluateToCons(data->tail)->head);
	} else
		return(node);
}
AST::NodeT replace(AST::NodeT needle /* not Symbol */, AST::NodeT replacement, AST::NodeT haystack) /* intended for builtins only */ {
	AST::NodeT data = AST::makeCons(needle, AST::makeCons(replacement, NULL));
	return(mapTree(data, replaceSingleNode, haystack));
}
AST::NodeT reduce1(AST::NodeT term) {
	if(GUI::interrupted_P())
		throw EvaluationException("evaluation was interrupted");
	if(recursionLevel > 10000) {
		recursionLevel = 0;
		throw EvaluationException("recursion was too deep");
	}
	if(application_P(term)) {
		if(application_result_P(term))
			return(((AST::Application*) term)->result);

		AST::NodeT fn;
		AST::NodeT argument;
		++recursionLevel;
		fn = reduce1(get_application_operator(term));
		--recursionLevel;
		argument = get_application_operand(term);
		if(wants_its_argument_reduced_P(fn)) {
			++recursionLevel;
			argument = reduce1(argument);
			--recursionLevel;
		}
		if(abstraction_P(fn)) {
			AST::NodeT body;
			body = get_abstraction_body(fn);
			body = shift(argument, 1, body);
			++recursionLevel;
			body = reduce1(body);
			--recursionLevel;
			return(remember(term, body));
		} else {
			// most of the time, SymbolReference anyway: AST::Symbol* fnName = dynamic_cast<AST::Symbol*>(fn);
			if(builtin_call_P(fn)) {
				AST::NodeT result;
				result = call_builtin(fn, argument);
				return(remember(term, result));
			} else {
				//std::string v = std::string("could not reduce ") + str(term);
				//throw Evaluators::EvaluationException(v.c_str());
				return(remember(term, ensureApplication(term, fn, argument)));
			}
		}
	} else if(abstraction_P(term)) {
		/* this isn't strictly necessary, but nicer */
		AST::NodeT body;
		AST::NodeT parameter;
		AST::NodeT new_body;
		new_body = body = get_abstraction_body(term);
		parameter = get_abstraction_parameter(term);
#if 0
		try {
			new_body = reduce1(body);
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
AST::NodeT close(AST::Symbol* parameter, AST::NodeT argument, AST::NodeT body) {
	return(makeApplication(makeAbstraction(parameter, body), argument));
}
AST::NodeT makeError(const char* reason) {
	// FIXME!!
	return(AST::makeStr(reason));
}
bool define_P(AST::NodeT input) {
	return(input != NULL && application_P(input) && get_application_operator(input) == Symbols::Sdefine);
}
AST::NodeT evaluate(AST::NodeT computation) {
	if(application_P(computation))
		return(get_application_result(computation));
	else
		return(computation);
}
AST::Cons* evaluateToCons(AST::NodeT computation) {
	return(dynamic_cast<AST::Cons*>(evaluate(computation))); // TODO error check
}
AST::NodeT programFromSExpression(AST::NodeT root) {
	AST::Cons* consNode = dynamic_cast<AST::Cons*>(root);
	if(consNode) {
		// application or abstraction
		if(consNode->head == Symbols::Sbackslash) { // abstraction
			assert(consNode->tail);
			assert(evaluateToCons(consNode->tail)->tail);
			assert(evaluateToCons(evaluateToCons(consNode->tail)->tail)->tail == NULL);
			AST::NodeT parameter = evaluateToCons(consNode->tail)->head;
			AST::NodeT body = evaluateToCons(evaluateToCons(consNode->tail)->tail)->head;
			return(makeAbstraction(programFromSExpression(parameter), programFromSExpression(body)));
		} else { // application
			assert(evaluateToCons(consNode->tail));
			assert(evaluateToCons(evaluateToCons(consNode->tail)->tail) == NULL);
			AST::NodeT operator_ = consNode->head;
			AST::NodeT operand = evaluateToCons(consNode->tail)->head;
			return(makeApplication(programFromSExpression(operator_), programFromSExpression(operand)));
		}
	} else
		return(root);
}

REGISTER_BUILTIN(Reducer, 1, 0, Symbols::Sinline)
REGISTER_BUILTIN(Quoter, 1, 0, Symbols::Squote)

AST::NodeT quote(AST::NodeT value) {
	return(makeApplication(&Quoter, value));
}

CurriedOperation* makeCurriedOperation(NodeT operation, NodeT argument) {
	CurriedOperation* result = new CurriedOperation;
	result->fOperation  = operation;
	result->fArgument = argument;
	return(result);
}

}; // end namespace Evaluators.
