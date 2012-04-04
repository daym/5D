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
	AST::Node* fEnvironmentFrontier; // see tail
};
static bool envEntry_P(AST::Node* node) {
	return(application_P(node) && abstraction_P(get_application_operator(node)));
}
static void envEntrySetTail(AST::Node* node, AST::Node* value) {
	// continue at the abstraction body embedded in the application (all other slots are used up)
	assert(envEntry_P(node));
	AST::Node* op = get_application_operator(node);
	assert(abstraction_P(op));
	((AST::Abstraction*)op)->body = value; // shoot me now
}
static AST::Node* envEntryGetTail(AST::Node* node) {
	// continue at the abstraction body embedded in the application (all other slots are used up)
	assert(envEntry_P(node));
	AST::Node* op = get_application_operator(node);
	assert(abstraction_P(op));
	return(get_abstraction_body(op));
}
static AST::Node* makeEnvEntry(AST::Symbol* name, AST::Node* body, AST::Node* next) {
	return(makeApplication(makeAbstraction(name, next), body));
}
static void getEnvEntry(AST::Node* entry, AST::Node*& name, AST::Node*& body, AST::Node*& next) {
	assert(application_P(entry));
	AST::Node* abstraction = get_application_operator(entry);
	assert(abstraction_P(abstraction));
	name = get_abstraction_parameter(abstraction);
	next = get_abstraction_body(abstraction);
	body = get_application_operand(entry);
}
AST::Node* REPL_get_definition_backwards(struct REPL* self, AST::Symbol* name, size_t backOffset) {
	// envEntrySetTail(self->fTailUserEnvironmentFrontier, NULL);
	AST::Node* x_name;
	AST::Node* x_body;
	AST::Node* x_next;
	std::deque<AST::Node*> matches;
	for(AST::Node* entry = envEntryGetTail(self->fEnvironmentTail); getEnvEntry(entry, x_name, x_body, x_next), true; entry = x_next) {
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
static AST::Node* REPL_close_environment(struct REPL* self, AST::Node* node) {
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
AST::Node* describe(struct REPL* self, AST::Node* options, AST::Node* key) {
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
	AST::Node* backOffsetNode = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("@backOffset:"));
	int backOffset = backOffsetNode ? Evaluators::get_int(backOffsetNode) : 0;
	if(dynamic_cast<AST::Symbol*>(argument) != NULL) {
		AST::Node* body = REPL_get_definition_backwards(self, dynamic_cast<AST::Symbol*>(argument), backOffset);
		return(body);
	} else
		return(argument);
}
AST::Node* define(struct REPL* self, AST::Node* key, AST::Node* value) {
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
	AST::Node* name = iter->second;
	++iter;
	assert(iter != arguments.end());
	AST::Node* body = iter->second;
	/* make sure it would actually work... (otherwise would throw exception) */
	REPL_prepare(self, body);
	REPL_add_to_environment(self, name, body);
	return(name);
}
AST::Node* import(struct REPL* self, AST::Node* options, AST::Node* filename) {
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
	AST::Node* filename = iter->second;
	if(!str_P(filename))
		throw Evaluators::EvaluationException("import: expected Str filename");
	AST::Node* body = Evaluators::import_module(options/*FIXME*/, filename);
	//body = Evaluators::annotate(body);
	//REPL_prepare(self, body);
	//REPL_add_to_environment(self, name, body);

	// TODO make it possible to limit imports (use whitelist and blacklist)
	AST::Node* whitelistN = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("whitelist:"));
	AST::HashTable whitelist = setFromList(whitelistN);
	AST::Node* blacklistN = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("blacklist:"));
	AST::HashTable blacklist = setFromList(blacklistN);
	AST::Node* prefixN = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("prefix:"));
	char* prefix = prefixN ? Evaluators::get_string(prefixN) : NULL;

	AST::Node* exports = Evaluators::reduce(AST::makeApplication(body, Symbols::Sexports));
	AST::Cons* usedExports = NULL;
	blacklist[Symbols::Sexports->name] = NULL;
	for(AST::Cons* node = Evaluators::evaluateToCons(exports); node; node = Evaluators::evaluateToCons(node->tail)) {
		AST::Symbol* xname = dynamic_cast<AST::Symbol*>(node->head);
		if(xname && (whitelist.empty() || whitelist.find(xname->name) != whitelist.end()) && blacklist.find(xname->name) == blacklist.end()) {
			AST::Node* xbody = AST::makeApplication(body, Evaluators::quote(xname));
			//REPL_prepare(self, xbody);
			if(prefix && prefix[0]) {
				std::stringstream sst;
				sst << prefix << xname->name;
				std::string v = sst.str();
				xname = AST::symbolFromStr(v.c_str());
			}
			REPL_add_to_environment(self, xname, xbody);
			usedExports = AST::makeCons(xname, usedExports);
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
AST::Node* evaluate(struct REPL* self, AST::Node* node) {
	/* TODO:
	- eval the node in the environment fEnvironment.
	- if possible, avoid having to copy over all of the fEnvironment nodes every time (possibly by having a copy where the actual environment lives in).
	*/
	bool B_ok = false;
	try {
		AST::Node* result = input;
		Evaluators::resetWorld();
		result = prepare_module(REPL_close_environment(self, result)));
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
