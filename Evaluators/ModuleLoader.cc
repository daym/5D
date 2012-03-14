#include <stdio.h>
#include "Evaluators/ModuleLoader"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Scanners/OperatorPrecedenceList"
#include "AST/AST"
#include "AST/HashTable"
#include "FFIs/FFIs"
#include "Scanners/MathParser"
#include "FFIs/Allocators"
#include "FFIs/ProcessInfos"

namespace Evaluators {

Scanners::OperatorPrecedenceList* default_operator_precedence_list(void) {
	Scanners::OperatorPrecedenceList* result;
	result = new Scanners::OperatorPrecedenceList;
	// FIXME just reuse the same global all the time.
	return(result);
}
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
	AST::Node* result = Evaluators::provide_dynamic_builtins(input);
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
	try {
		Scanners::MathParser parser;
		FILE* input_file = fopen(filename, "rb");
		if(!input_file)
			return(FALLBACK);
		try {
			parser.push(input_file, 0, filename);
			AST::Node* result = NULL;
			result = parser.parse(default_operator_precedence_list(), Symbols::SlessEOFgreater);
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
static std::vector<std::string> get_module_search_path(void) {
	std::vector<std::string> result;
#ifdef WIN32
	result.push_back(".\\");
	result.push_back(get_shared_dir());
#else
	result.push_back("");
	result.push_back(get_shared_dir());
#endif
	return(result);
}
AST::Node* import_module(AST::Node* options, AST::Node* fileNameNode) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, fileNameNode);
	std::vector<std::string> searchPaths = get_module_search_path();
	std::vector<std::string>::const_iterator endSearchPaths = searchPaths.end();
	std::string filename = "";
	std::string realFilename = "";
	std::string moduleKey = "";
	if(FFIs::absolute_path_P(dynamic_cast<AST::Str*>(fileNameNode))) {
		filename = Evaluators::get_string(fileNameNode);
		moduleKey = getModuleFileKey(filename, realFilename);
	} else for(std::vector<std::string>::const_iterator iterSearchPaths = searchPaths.begin(); iterSearchPaths != endSearchPaths; ++iterSearchPaths) {
		std::stringstream filenameStream;
		filenameStream << *iterSearchPaths;
		filenameStream << Evaluators::get_string(fileNameNode);
		filename = filenameStream.str();
		moduleKey = getModuleFileKey(filename, realFilename);
		if(moduleKey.length() != 0)
			break;
	}
	if(moduleKey.length() == 0 || realFilename.length() == 0) {
		return(FALLBACK);
	}
	char* actualFilename = FFIs::get_absolute_path(realFilename.c_str());
	AST::Node* body = Evaluators::require_module(actualFilename, moduleKey);
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&RModule, body)), AST::makeStr(actualFilename))));
}

DEFINE_FULL_OPERATION(RModule, {
	return(access_module(fn, argument));
})
DEFINE_FULL_OPERATION(RModuleLoader, {
	return(import_module(fn, argument));
})

REGISTER_BUILTIN(RModule, 3, 1, AST::symbolFromStr("requireModule"));
REGISTER_BUILTIN(RModuleLoader, 1, 1, AST::symbolFromStr("requireModule"));

static std::string sharedDir = PREFIX "/share/5D/"; // keep "/" suffix.
std::string get_shared_dir(void) {
	return(sharedDir);
}
void set_shared_dir(const std::string& name) {
	sharedDir = name;
}
void set_shared_dir_by_executable(const char* argv0) {
	/* if the program was found in search path, this isn't worth much. */
	/*
	sharedDir = argv0;
	std::string::size_type slashPos = sharedDir.find_last_of('/');
	if(slashPos != std::string::npos) {
		sharedDir = sharedDir.substr(0, slashPos + 1);
		sharedDir += "../share/5D/"; // TODO version
	} else {
		sharedDir = "";
	}*/
}

};
