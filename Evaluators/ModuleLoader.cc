#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include "Evaluators/ModuleLoader"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Scanners/OperatorPrecedenceList"
#include "Values/Values"
#include "FFIs/FFIs"
#include "Scanners/MathParser"
#include "FFIs/Allocators"
#include "FFIs/ProcessInfos"
#include "Evaluators/BuiltinSelector"
#include "Evaluators/FFI"

namespace Evaluators {
using namespace Values;

Scanners::OperatorPrecedenceList* default_operator_precedence_list(void) {
	static Scanners::OperatorPrecedenceList* result;
	if(!result)
		result = new Scanners::OperatorPrecedenceList;
	return(result);
}
static NodeT access_module(NodeT fn, NodeT argument) {
	NodeT body = get_curried_operation_argument(get_curried_operation_operation(fn));
	NodeT result = Evaluators::reduce(makeApplication(body, argument));
	return(result);
}

NodeT prepare_module(NodeT input) {
	NodeT result = Evaluators::provide_dynamic_builtins(input);
	result = Evaluators::annotate(result);
	return(result);
}

static NodeT force_import_module(const char* filename) {
	int previousErrno = errno;
	NodeT result = NULL;
	if(FFIs::sharedLibraryFileP(filename)) {
		return Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&FFIs::SharedLibraryLoader, makeStr(filename))), NULL)), makeStr("access5DModuleV1")));
	}
	try {
		Scanners::MathParser parser;
		FILE* input_file = fopen(filename, "rb");
		if(!input_file)
			return(FALLBACK);
		try {
			parser.push(input_file, 0, filename);
			NodeT result = NULL;
			result = parser.parse(default_operator_precedence_list(), Symbols::SlessEOFgreater);
			result = Evaluators::close(Symbols::Squote, &Evaluators::Quoter, result); // module needs that, so provide it.
			result = Evaluators::close(Symbols::Sdot, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, makeApplication(Symbols::Sa, Symbols::Sb))), result);
			result = Evaluators::close(Symbols::Shashexclam, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, Symbols::Sb)), result); // rem
			result = Evaluators::close(Symbols::Scolon, &Evaluators::Conser, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::Scomma, &Evaluators::Pairer, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::Snil, NULL, result); // dispatch [] needs that, so provide it.
			result = Evaluators::close(Symbols::SrequireModule, &ModuleLoader, result); // module needs that, so provide it. // TODO maybe use Builtins.requireModule (not sure whether that's useful)
			result = Evaluators::close(Symbols::SBuiltins, &Evaluators::BuiltinGetter, result);
			// ?? result = Evaluators::close(Symbols::SdispatchModule, makeAbstraction(Symbols::Sexports, makeApplication(&Evaluators::ModuleDispatcher, makeApplication(&Evaluators::ModuleBoxMaker, Symbols::Sexports))), result);
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
		throw;
	} catch(Scanners::ParseException& e) {
		std::string v = e.what() ? e.what() : "error";
		fprintf(stderr, "%s\n", v.c_str());
		throw;
	}
	return(makeAbstraction(symbolFromStr("name"), result));
}
static HashTable* fModules = new HashTable;
NodeT require_module(const char* filename, const std::string& xmoduleKey) {
	if(fModules == NULL) { /* init order problems, sigh. */
		fModules = new HashTable;
	}
	const char* moduleKeyC = xmoduleKey.c_str();
	char* moduleKey = GCx_strdup(moduleKeyC);
	if((*fModules).find(moduleKey) == fModules->end()) {
		(*fModules)[moduleKey] = symbolFromStr("loading"); // protect against endless recusion.
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
NodeT import_module(NodeT options, NodeT fileNameNode) {
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
	NodeT body = Evaluators::require_module(actualFilename, moduleKey);
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&Module, body)), makeStr(actualFilename))));
}

DEFINE_FULL_OPERATION(Module, {
	return(access_module(fn, argument));
})
DEFINE_FULL_OPERATION(ModuleLoader, {
	return(import_module(fn, argument));
})

REGISTER_BUILTIN(Module, 3, 1, symbolFromStr("requireModule"));
REGISTER_BUILTIN(ModuleLoader, 1, 1, symbolFromStr("requireModule"));

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

static NodeT mapGetFst2(NodeT fallback, Cons* c) {
	if(c == NULL) {
		NodeT tail = fallback ? reduce(makeApplication(fallback, Symbols::Sexports)) : NULL;
		return(tail);
	} else
		return(makeCons(Evaluators::get_pair_first(reduce(evaluateToCons(reduce(get_cons_head(c))))), mapGetFst2(fallback, evaluateToCons(get_cons_tail(c)))));
}
static NodeT dispatchModule(NodeT options, NodeT argument) {
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
	Box* mBox = dynamic_cast<Box*>(iter->second);
	++iter;
	NodeT fallback = iter->second;
	++iter;
	NodeT key = iter->second;
	HashTable* m;
	if(!mBox) {
		throw Evaluators::EvaluationException("invalid symbol table entry (*)");
		return(NULL);
	}
	void* pBox = Evaluators::get_pointer(mBox);
	if(dynamic_cast<HashTable*>((NodeT) pBox) == NULL) {
		m = new (UseGC) HashTable;
		for(Cons* table = (Cons*) pBox; table; table = Evaluators::evaluateToCons(table->tail)) {
			std::string irv = Evaluators::str(table->head);
			//printf("irv %s\n", irv.c_str());
			Cons* entry = evaluateToCons(reduce(get_cons_head(table)));
			//std::string v = str(entry);
			//printf("=irv> %s\n", v.c_str());
			NodeT x_key = reduce(Evaluators::get_pair_first(entry));
			//std::string vkey = Evaluators::str(x_key);
			//printf("=key> %s\n", vkey.c_str());
			NodeT value = reduce(Evaluators::get_pair_second(entry));
			const char* name = get_symbol1_name(x_key);
			if(name && m->find(name) == m->end())
				(*m)[name] = value;
		}
		Cons* table = (Cons*) pBox;
		set_box_value(mBox, m);
		(*m)["exports"] = makeCons(Symbols::Sexports, mapGetFst2(fallback, table));
	}
	m = (HashTable*) mBox->value;
	//std::string vkey = Evaluators::str(key);
	//printf("%s\n", vkey.c_str());
	const char* name = get_symbol_name(key); 
	if(name) {
		/*HashTable::const_iterator b = m->begin();
		HashTable::const_iterator e = m->end();
		for(; b != e; ++b) {
			printf("%s<\n", b->first);
		}
		printf("searching \"%s\"\n", s->name);*/
		HashTable::const_iterator iter = m->find(name);
		if(iter != m->end())
			return(iter->second);
		else {
			if(fallback) { // this should always hold
				return(reduce(makeApplication(fallback, key)));
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
static NodeT build_hash_exports(NodeT node) {
	if(nil_P(node))
		return(node);
	NodeT head = get_cons_head(node);
	NodeT tail = Evaluators::evaluateToCons(get_cons_tail(node));
	NodeT result = makeApplication(makeApplication(&Evaluators::Pairer, makeApplication(&Evaluators::Quoter, head)), head);
	return makeCons(result, build_hash_exports(tail));
}
/*
better:

parseExports! = (\world
	parseValue! world ;\value
	return! (map (\item ((quote item), item)) (eval value [('(:), (:)) ('nil, nil) ('(,), (,)]))
) in 
*/
static NodeT hashExports(NodeT options, NodeT argument) {
	NodeT result = Evaluators::reduce(argument);
	std::string s = Evaluators::str(result);
	printf("%s\n", s.c_str());
	return(makeApplication(Evaluators::get_module_entry_accessor("Composition", Symbols::Sdispatch), build_hash_exports(result)));
}
DEFINE_FULL_OPERATION(ModuleDispatcher, return(dispatchModule(fn, argument));)
DEFINE_FULL_OPERATION(HashExporter, return(hashExports(fn, argument));)
static inline NodeT makeModuleBox(NodeT argument) {
	return makeBox(argument, makeApplication(&ModuleBoxMaker, argument));
}
DEFINE_SIMPLE_OPERATION(ModuleBoxMaker, makeModuleBox(reduce(argument)))
REGISTER_BUILTIN(ModuleDispatcher, (-3), 1, symbolFromStr("dispatchModule"))
REGISTER_BUILTIN(ModuleBoxMaker, 1, 0, symbolFromStr("makeModuleBox"))
REGISTER_BUILTIN(HashExporter, 1, 0, symbolFromStr("#exports"))

};
