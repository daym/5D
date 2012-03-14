#include "Evaluators/Modules"
#include "Scanners/OperatorPrecedenceList"
#include "AST/AST"
#include "AST/HashTable"

namespace Evaluators {
static AST::Node* access_module(AST::Node* fn, AST::Node* argument) {
	Evaluators::CurriedOperation* o = dynamic_cast<Evaluators::CurriedOperation*>(fn);
	Evaluators::CurriedOperation* o2 = dynamic_cast<Evaluators::CurriedOperation*>(o->fOperation);
	AST::Node* body = o2->fArgument;
	// filename is the second argument, so ignore.
	std::string v = Evaluators::str(o->fArgument);
	//fprintf(stderr, "accessing module %s\n", v.c_str());
	AST::Node* result = Evaluators::reduce(AST::makeApplication(body, argument));
	//fprintf(stderr, "end accessing module %s\n", v.c_str());
	return(result);
}

AST::Node* prepare_module(AST::Node* input) {
	result = Evaluators::provide_dynamic_builtins(input);
	result = Evaluators::annotate(result);
	return(result);
}

static AST::Node* force_import_module(const char* filename) {
	// FIXME push symbol table
	int previousErrno = errno;
	AST::Node* result = NULL;
	if(FFIs::sharedLibraryFileP(filename)) {
		return Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&FFIs::SharedLibraryLoader, AST::makeStr(filename))), NULL)), AST::makeStr("access5DModuleV1")));
	}
	// TODO in principle, this could just be loaded into a new REPL.
	try {
		Scanners::MathParser parser;
		FILE* input_file = fopen(filename, "rb");
		if(!input_file)
			return(FALLBACK);
		try {
			parser.push(input_file, 0, filename);
			AST::Node* result = NULL;
			result = parser.parse(REPL_ensure_operator_precedence_list(self), Symbols::SlessEOFgreater);
			result = Evaluators::close(Symbols::Scolon, &Evaluators::Conser, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::SrequireModule, &RModuleLoader, result); // module needs that, so provide it.
			// ?? result = Evaluators::close(Symbols::SdispatchModule, AST::makeAbstraction(Symbols::Sexports, AST::makeApplication(&Evaluators::ModuleDispatcher, AST::makeApplication(&Evaluators::ModuleBoxMaker, Symbols::Sexports))), result);
			result = prepare_module(result);
			//fprintf(stderr, "before reduce_module\n");
			result = Evaluators::reduce(result);
			//fprintf(stderr, "after reduce_module\n");
			fclose(input_file);
			errno = previousErrno;
			return(result);
		} catch(...) {
			fclose(input_file);
			errno = previousErrno;
			throw;
		}
	} catch(Evaluators::EvaluationException e) {
		std::string v = e.what() ? e.what() : "error";
		v = " => " + v + "\n";
		fprintf(stderr, "%s\n", v.c_str());
	} catch(Scanners::ParseException& e) {
		std::string v = e.what() ? e.what() : "error";
		fprintf(stderr, "%s\n", v.c_str());
	}
	// FIXME pop symbol table
	return(AST::makeAbstraction(AST::symbolFromStr("name"), result));
	// FIXME return(uncurried(&RModule, AST::makeStr(filename)));
}
static AST::HashTable* fModules = new AST::HashTable;
AST::Node* require_module(const char* filename, const std::string& xmoduleKey) {
	if(fModules == NULL) { /* init order problems, sigh. */
		fModules = new AST::HashTable;
	}
	const char* moduleKeyC = xmoduleKey.c_str();
	char* moduleKey = GCx_strdup(moduleKeyC);
	if((*fModules).find(moduleKey) == fModules->end()) {
		(*fModules)[moduleKey] = AST::symbolFromStr("loading"); // protect against endless recusion.
		(*fModules)[moduleKey] = force_import_module(filename);
	}
	return((*fModules)[moduleKey]);
}

DEFINE_FULL_OPERATION(RModule, {
	return(access_module(fn, argument));
})
REGISTER_BUILTIN(RModule, 3, 1, AST::symbolFromStr("requireModule"));
REGISTER_BUILTIN(RModuleLoader, 1, 1, AST::symbolFromStr("requireModule"));

};
