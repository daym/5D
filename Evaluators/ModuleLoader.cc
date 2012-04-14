#ifdef WIN32
#include "stdafx.h"
#endif
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
#include "Evaluators/BuiltinSelector"

namespace Evaluators {

Scanners::OperatorPrecedenceList* default_operator_precedence_list(void) {
	static Scanners::OperatorPrecedenceList* result;
	if(!result)
		result = new Scanners::OperatorPrecedenceList;
	return(result);
}
static AST::NodeT access_module(AST::NodeT fn, AST::NodeT argument) {
	AST::NodeT body = get_curried_operation_argument(get_curried_operation_operation(fn));
	AST::NodeT result = Evaluators::reduce(AST::makeApplication(body, argument));
	return(result);
}

AST::NodeT prepare_module(AST::NodeT input) {
	AST::NodeT result = Evaluators::provide_dynamic_builtins(input);
	result = Evaluators::annotate(result);
	return(result);
}

static AST::NodeT force_import_module(const char* filename) {
	int previousErrno = errno;
	AST::NodeT result = NULL;
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
			AST::NodeT result = NULL;
			result = parser.parse(default_operator_precedence_list(), Symbols::SlessEOFgreater);
			result = Evaluators::close(Symbols::Squote, &Evaluators::Quoter, result); // module needs that, so provide it.
			result = Evaluators::close(Symbols::Sdot, AST::makeAbstraction(Symbols::Sa, AST::makeAbstraction(Symbols::Sb, AST::makeApplication(Symbols::Sa, Symbols::Sb))), result);
			result = Evaluators::close(Symbols::Scolon, &Evaluators::Conser, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::Snil, NULL, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::SrequireModule, &ModuleLoader, result); // module needs that, so provide it. // TODO maybe use Builtins.requireModule (not sure whether that's useful)
			result = Evaluators::close(Symbols::SBuiltins, &Evaluators::BuiltinGetter, result);
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
	return(AST::makeAbstraction(AST::symbolFromStr("name"), result));
}
static AST::HashTable* fModules = new AST::HashTable;
AST::NodeT require_module(const char* filename, const std::string& xmoduleKey) {
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
AST::NodeT import_module(AST::NodeT options, AST::NodeT fileNameNode) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, fileNameNode);
	std::vector<std::string> searchPaths = get_module_search_path();
	std::vector<std::string>::const_iterator endSearchPaths = searchPaths.end();
	std::string filename = "";
	std::string realFilename = "";
	std::string moduleKey = "";
	if(FFIs::absolute_path_P(fileNameNode)) {
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
	AST::NodeT body = Evaluators::require_module(actualFilename, moduleKey);
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&Module, body)), AST::makeStr(actualFilename))));
}

DEFINE_FULL_OPERATION(Module, {
	return(access_module(fn, argument));
})
DEFINE_FULL_OPERATION(ModuleLoader, {
	return(import_module(fn, argument));
})

REGISTER_BUILTIN(Module, 3, 1, AST::symbolFromStr("requireModule"));
REGISTER_BUILTIN(ModuleLoader, 1, 1, AST::symbolFromStr("requireModule"));

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

static AST::NodeT mapGetFst2(AST::NodeT fallback, AST::Cons* c) {
	if(c == NULL) {
		AST::NodeT tail = fallback ? reduce(AST::makeApplication(fallback, Symbols::Sexports)) : NULL;
		return(tail);
	} else
		return(AST::makeCons(AST::get_pair_first(reduce(evaluateToCons(reduce(c->head)))), mapGetFst2(fallback, evaluateToCons(c->tail))));
}
static AST::NodeT dispatchModule(AST::NodeT options, AST::NodeT argument) {
	/* parameters: <exports> <fallback> <key> 
	               2         1          0*/
	CXXArguments arguments = Evaluators::CXXfromArgumentsU(options, argument, 1);
	CXXArguments::const_iterator iter = arguments.begin();
	/*std::stringstream buffer;
	std::string v;
	int position = 0;
	Formatters::Math::print_CXX(new Scanners::OperatorPrecedenceList(), buffer, position, iter->second, 0, false);
	v = buffer.str();
	printf("%s\n", v.c_str());*/
	AST::Box* mBox = dynamic_cast<AST::Box*>(iter->second);
	++iter;
	AST::NodeT fallback = iter->second;
	++iter;
	AST::NodeT key = iter->second;
	AST::HashTable* m;
	if(!mBox) {
		throw Evaluators::EvaluationException("invalid symbol table entry (*)");
		return(NULL);
	}
	if(dynamic_cast<AST::HashTable*>((AST::NodeT) mBox->native) == NULL) {
		//cons_P((AST::NodeT) mBox->native)) {
		m = new (UseGC) AST::HashTable;
		for(AST::Cons* table = (AST::Cons*) mBox->native; table; table = Evaluators::evaluateToCons(table->tail)) {
			AST::Cons* entry = evaluateToCons(reduce(table->head));
			//std::string v = str(entry);
			//printf("%s\n", v.c_str());
			AST::NodeT x_key = reduce(entry->head);
			AST::Cons* snd = evaluateToCons(entry->tail);
			if(!snd)
				throw Evaluators::EvaluationException("invalid symbol table entry");
			AST::NodeT value = reduce(snd->head);
			const char* name = AST::get_symbol1_name(x_key);
			if(m->find(name) == m->end())
				(*m)[name] = value;
		}
		AST::Cons* table = (AST::Cons*) mBox->native;
		mBox->native = m;
		(*m)["exports"] = AST::makeCons(Symbols::Sexports, mapGetFst2(fallback, table));
	}
	m = (AST::HashTable*) mBox->native;
	const char* name = AST::get_symbol1_name(key); 
	if(name) {
		/*HashTable::const_iterator b = m->begin();
		HashTable::const_iterator e = m->end();
		for(; b != e; ++b) {
			printf("%s<\n", b->first);
		}
		printf("searching \"%s\"\n", s->name);*/
		AST::HashTable::const_iterator iter = m->find(name);
		if(iter != m->end())
			return(iter->second);
		else {
			if(fallback) { // this should always hold
				return(reduce(AST::makeApplication(fallback, key)));
			} else { // this is a leftover
				std::stringstream sst;
				sst << "library does not contain '" << name;
				std::string v = sst.str();
				throw Evaluators::EvaluationException(GCx_strdup(v.c_str()));
			}
		}
	} else
		throw Evaluators::EvaluationException("not a symbol");
	return(NULL);
}
static AST::NodeT build_hash_exports(AST::NodeT node) {
	if(node == Symbols::Snil)
		return(node);
	if(!AST::application_P(node) || !AST::application_P(AST::get_application_operator(node)))
		return(nil); // XXX
	AST::NodeT aoperation = AST::get_application_operator(node);
	if(AST::get_application_operator(aoperation) != Symbols::Scolon)
		return(nil);
	AST::NodeT head = AST::get_application_operand(aoperation);
	AST::NodeT tail = AST::get_application_operand(node);
	std::string heads = Evaluators::str(head);
	printf("%s\n", heads.c_str());
	AST::NodeT result = AST::makeApplication(AST::makeApplication(&Evaluators::Pairer, AST::makeApplication(&Evaluators::Quoter, head)), head);
	return AST::makeApplication(AST::makeApplication(Symbols::Scolon, result), build_hash_exports(tail));
}
/*
better:

parseExports! = (\world
	parseValue! world ;\value
	return! (map (\item ((quote item), item)) (eval value [('(:), (:)) ('nil, nil) ('(,), (,)]))
) in 
*/
static AST::NodeT hashExports(AST::NodeT options, AST::NodeT argument) {
	AST::NodeT result = argument; // DO NOT REDUCE
	return(AST::makeApplication(Evaluators::get_module_entry_accessor("Composition", Symbols::Sdispatch), build_hash_exports(result)));
}
DEFINE_FULL_OPERATION(ModuleDispatcher, return(dispatchModule(fn, argument));)
DEFINE_FULL_OPERATION(HashExporter, return(hashExports(fn, argument));)
static inline AST::NodeT makeModuleBox(AST::NodeT argument) {
	return AST::makeBox(argument, AST::makeApplication(&ModuleBoxMaker, argument));
}
DEFINE_SIMPLE_OPERATION(ModuleBoxMaker, makeModuleBox(reduce(argument)))
REGISTER_BUILTIN(ModuleDispatcher, (-3), 1, AST::symbolFromStr("dispatchModule"))
REGISTER_BUILTIN(ModuleBoxMaker, 1, 0, AST::symbolFromStr("makeModuleBox"))
REGISTER_BUILTIN(HashExporter, 1, 0, AST::symbolFromStr("#exports"))

};
