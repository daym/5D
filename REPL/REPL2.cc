#include "REPL/REPL2"
#include "REPL/Environment"

namespace REPL {

/* this file is unused */
/* dynamic scope */

struct REPL {
	Scanners::OperatorPrecedenceList* fOPL;
	Environment* fValenceEnvironment;
	NodeT fEnvironment;
	NodeT fEnvironmentTail;
	NodeT fEnvironmentFrontier; // see tail
};
static bool envEntry_P(NodeT node) {
	return(application_P(node) && abstraction_P(get_application_operator(node)));
}
static void envEntrySetTail(NodeT node, NodeT value) {
	// continue at the abstraction body embedded in the application (all other slots are used up)
	assert(envEntry_P(node));
	NodeT op = get_application_operator(node);
	assert(abstraction_P(op));
	((Abstraction*)op)->body = value; // shoot me now
}
static NodeT envEntryGetTail(NodeT node) {
	// continue at the abstraction body embedded in the application (all other slots are used up)
	assert(envEntry_P(node));
	NodeT op = get_application_operator(node);
	assert(abstraction_P(op));
	return(get_abstraction_body(op));
}
static NodeT makeEnvEntry(Symbol* name, NodeT body, NodeT next) {
	return(makeApplication(makeAbstraction(name, next), body));
}
static void getEnvEntry(NodeT entry, NodeT& name, NodeT& body, NodeT& next) {
	assert(application_P(entry));
	NodeT abstraction = get_application_operator(entry);
	assert(abstraction_P(abstraction));
	name = get_abstraction_parameter(abstraction);
	next = get_abstraction_body(abstraction);
	body = get_application_operand(entry);
}
NodeT REPL_get_definition_backwards(struct REPL* self, Symbol* name, size_t backOffset) {
	// envEntrySetTail(self->fTailUserEnvironmentFrontier, NULL);
	NodeT x_name;
	NodeT x_body;
	NodeT x_next;
	std::deque<NodeT> matches;
	for(NodeT entry = envEntryGetTail(self->fEnvironmentTail); getEnvEntry(entry, x_name, x_body, x_next), true; entry = x_next) {
		if(x_name == name) {
			matches.push_front(x_body);
		}
		if(entry == fEnvironmentFrontier)
			break;
	}
	if(backOffset >= 0 && backOffset < matches.size())
		return(matches[backOffset]);
	else
		return(NULL);
}
static NodeT REPL_close_environment(struct REPL* self, NodeT node) {
	if(self->fTailUserEnvironmentFrontier) {
		if(increaseGeneration() == 1) { /* overflow */
			mapTree(NULL, uncacheNodeResult, Evaluators::evaluateToCons(self->fTailUserEnvironment)->tail);
		}
		envEntrySetTail(self->fTailUserEnvironmentFrontier, node);
		return(envEntryGetTail(self->fTailEnvironment));
	} else
		return(node);
}
OperatorPrecedenceList* getOperatorPrecedenceList(struct REPL* self) {
	return(self->fOPL);
}
void regenerateActualEnvironment(struct REPL* self) {
	/* fValenceEnvironment -> fEnvironment */
}
Environment* getEnvironment(struct REPL* self) {
	return(self->fValenceEnvironment);
}
NodeT describe(struct REPL* self, NodeT options, NodeT key) {
	/*
	- find the item in the environment, if any.
	- print the current value, or the one at backOffset specified in #options. */
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	struct REPL* self = dynamic_cast<struct REPL*>(arguments.front().second);
	assert(self);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	assert(iter != arguments.end());
	++iter;
	assert(iter != arguments.end());
	argument = iter->second;
	NodeT backOffsetNode = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("@backOffset:"));
	int backOffset = backOffsetNode ? Evaluators::get_int(backOffsetNode) : 0;
	if(dynamic_cast<Symbol*>(argument) != NULL) {
		NodeT body = REPL_get_definition_backwards(self, dynamic_cast<Symbol*>(argument), backOffset);
		return(body);
	} else
		return(argument);
}
NodeT define(struct REPL* self, NodeT key, NodeT value) {
	/* TODO:
	- place the item in the environment, shifting existing nodes as necessary. 
	regenerateActualEnvironment() 
	*/
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	struct REPL* self = dynamic_cast<struct REPL*>(arguments.front().second);
	assert(self);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	assert(iter != arguments.end());
	++iter;
	assert(iter != arguments.end());
	NodeT name = iter->second;
	++iter;
	assert(iter != arguments.end());
	NodeT body = iter->second;
	/* make sure it would actually work... (otherwise would throw exception) */
	REPL_prepare(self, body);
	REPL_add_to_environment(self, name, body);
	return(name);
}
NodeT import(struct REPL* self, NodeT options, NodeT filename) {
	/* TODO:
	- mass-place the item in the environment.
	regenerateActualEnvironment() 
	*/
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	struct REPL* self = dynamic_cast<struct REPL*>(iter->second);
	assert(self);
	++iter;
	assert(iter != arguments.end());
	NodeT filename = iter->second;
	if(!str_P(filename))
		throw Evaluators::EvaluationException("import: expected Str filename");
	NodeT body = Evaluators::import_module(options/*FIXME*/, filename);
	//body = Evaluators::annotate(body);
	//REPL_prepare(self, body);
	//REPL_add_to_environment(self, name, body);

	// TODO make it possible to limit imports (use whitelist and blacklist)
	NodeT whitelistN = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("whitelist:"));
	HashTable whitelist = setFromList(whitelistN);
	NodeT blacklistN = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("blacklist:"));
	HashTable blacklist = setFromList(blacklistN);
	NodeT prefixN = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("prefix:"));
	char* prefix = prefixN ? Evaluators::get_string(prefixN) : NULL;

	NodeT exports = Evaluators::reduce(makeApplication(body, Symbols::Sexports));
	Cons* usedExports = NULL;
	blacklist[Symbols::Sexports->name] = NULL;
	for(Cons* node = Evaluators::evaluateToCons(exports); node; node = Evaluators::evaluateToCons(node->tail)) {
		Symbol* xname = dynamic_cast<Symbol*>(node->head);
		if(xname && (whitelist.empty() || whitelist.find(xname->name) != whitelist.end()) && blacklist.find(xname->name) == blacklist.end()) {
			NodeT xbody = makeApplication(body, Evaluators::quote(xname));
			//REPL_prepare(self, xbody);
			if(prefix && prefix[0]) {
				std::stringstream sst;
				sst << prefix << xname->name;
				std::string v = sst.str();
				xname = symbolFromStr(v.c_str());
			}
			REPL_add_to_environment(self, xname, xbody);
			usedExports = makeCons(xname, usedExports);
		}
	}
	return(usedExports);
	return(REPL_import(self, filename));
}
void purge(struct REPL* self) {
	/* TODO:
	- traverse the hashtable, saving live items.
	*/
}
NodeT evaluate(struct REPL* self, NodeT node) {
	/* TODO:
	- eval the node in the environment fEnvironment.
	- if possible, avoid having to copy over all of the fEnvironment nodes every time (possibly by having a copy where the actual environment lives in).
	*/
	bool B_ok = false;
	try {
		NodeT result = input;
		Evaluators::resetWorld();
		result = prepareModule(REPL_close_environment(self, result)));
		result = Evaluators::reduce(result);
		/*std::string v = result ? result->str() : "OK";
		v = " => " + v + "\n";
		REPL_insert_into_output_buffer(self, destination, v.c_str());*/
		REPL_enqueue_LATEX(self, result, destination);
		B_ok = true;
	} catch(Evaluators::EvaluationException e) {
		std::string v = e.what() ? e.what() : "error";
		v = " => " + v + "\n";
		REPL_insert_into_output_buffer(self, destination, v.c_str());
	}
	REPL_set_file_modified(self, true);
	return(B_ok);
}

}; /* end namespace REPL */
