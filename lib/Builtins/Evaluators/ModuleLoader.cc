#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <5D/FFIs>
#include "Evaluators/ModuleLoader"
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"
#include "Values/Values"
#include "FFIs/FFIs"
#include "Scanners/MathParser"
#include <5D/Allocators>
#include "FFIs/ProcessInfos"
#include "Evaluators/FFI"
#include "ModuleSystem/Modules"
#include "Formatters/Math"
#include <5D/ModuleSystem>

/* manages multiple modules */

namespace Evaluators {
using namespace Values;
using namespace ModuleSystem;
using namespace FFIs;

static inline std::string getModuleFileKey(const std::string& filename, std::string& outActualFilename) {
	int previousErrno = errno;
	outActualFilename = "";
	std::stringstream moduleKeyStream;
#ifdef WIN32
#ifndef S_IFDIR
#define S_IFDIR _S_IFDIR
#endif
#ifndef S_IFMT
#define S_IFMT (S_IFDIR|S_IFCHR|S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
	struct _stat buf;
	std::wstring filenameW = wstringFromUtf8(filename.c_str());
	if(_wstat(filenameW.c_str(), &buf) == -1) {
		//perror(filename.c_str());
		errno = previousErrno;
		return("");
	}
	if(S_ISDIR(buf.st_mode)) {
		outActualFilename = filename + "\\init.5D";
		filenameW = wstringFromUtf8(outActualFilename.c_str());
		if(_wstat(filenameW.c_str(), &buf) == -1) {
			//perror(filename.c_str());
			errno = previousErrno;
			return("");
		}
	} else
		outActualFilename = filename;
	moduleKeyStream << filename << ":"; /* on Windows, st_ino is 0, so HACK HACK HACK */
#else
	struct stat buf;
	if(stat(filename.c_str(), &buf) == -1) {
		//perror(filename.c_str());
		errno = previousErrno;
		return("");
	}
	if(S_ISDIR(buf.st_mode)) {
		outActualFilename = filename + "/init.5D";
		if(stat(outActualFilename.c_str(), &buf) == -1) {
			//perror(filename.c_str());
			errno = previousErrno;
			return("");
		}
	} else
		outActualFilename = filename;
#endif
	moduleKeyStream << buf.st_dev << ":" << buf.st_ino;
	std::string moduleKey = moduleKeyStream.str();
	errno = previousErrno;
	return(moduleKey);
}

static NodeT loadModule(NodeT options, NodeT fileNameNode);
NodeT selectOperatorPrecedenceList(NodeT shebang) {
	/* shebang is either NULL or a "command line string" as is UNIX tradition. */
	NodeT result = NULL;
	char* shebangS = stringOrNilFromNode(shebang);
	if(shebangS) {
		/* FIXME use an actual command line parsing library (if that exists) */
		const char* p = strstr(shebangS, "-p");
		if(p) {
			p = &p[3];
			char* r = GCx_strdup(p);
			char* q = strchr(r, ' ');
			if(q) {
				*q = 0;
			}
			if(*r == '"') { /* quoted string */
				++r;
				char* q = strrchr(r, '"');
				if(q)
					*q = 0;
			}
			result = getModuleEntryAccessor(r, Symbols::Stable);
			//result = loadModule(NULL, makeStr(r));
		}
	}
	return(result);
}
/** this is called to actually load a new module (as opposed to reusing an existing image) */
static NodeT forceModuleLoad(const char* filename) {
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
			NodeT shebang = parser.parseOptionalShebang();
			result = parser.parse(selectOperatorPrecedenceList(shebang), Symbols::SlessEOFgreater);
			/*fprintf(stderr, "\nbefore forceModuleLoad %s\n", filename);
			Formatters::Math::print(NULL, stderr, 0, 0, result);*/
			result = prepareModule(result, filename);
			//fprintf(stderr, "before reduce_module\n");
			result = Evaluators::reduce(result);
			/*fprintf(stderr, "\nafter forceModuleLoad\n");
			Formatters::Math::print(NULL, stderr, 0, 0, result);
			fprintf(stderr, "XXX\n");
			fflush(stderr);*/
			fclose(input_file);
			errno = previousErrno;
			return(result);
		} catch(...) {
			fclose(input_file);
			errno = previousErrno;
			throw;
		}
	} catch(const Evaluators::EvaluationException& e) {
		std::string v = e.what() ? e.what() : "error";
		v = " => " + v + "\n";
		fprintf(stderr, "%s\n", v.c_str());
		throw;
	} catch(const Scanners::ParseException& e) {
		std::string v = e.what() ? e.what() : "error";
		fprintf(stderr, "%s\n", v.c_str());
		throw;
	}
	return(makeAbstraction(symbolFromStr("name"), result));
}
static Hashtable* fModules = new Hashtable;
/* this isn't exactly public */
static NodeT requireModule(const char* filename, const std::string& xmoduleKey) {
	if(fModules == NULL) { /* init order problems, sigh. */
		fModules = new Hashtable;
	}
	const char* moduleKeyC = xmoduleKey.c_str();
	char* moduleKey = GCx_strdup(moduleKeyC);
	if((*fModules).find(moduleKey) == fModules->end()) {
		(*fModules)[moduleKey] = symbolFromStr("loading"); // protect against endless recusion.
		(*fModules)[moduleKey] = forceModuleLoad(filename);
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
	result.push_back("lib/");
	result.push_back(get_shared_dir());
#endif
	return(result);
}
/** this is the 5D interface for requireModule, called requireModule in 5D. */
static NodeT loadModule(NodeT options, NodeT fileNameNode) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, fileNameNode);
	std::vector<std::string> searchPaths = get_module_search_path();
	std::vector<std::string>::const_iterator endSearchPaths = searchPaths.end();
	std::string filename = "";
	std::string realFilename = "";
	std::string moduleKey = "";
	if(FFIs::absolute_path_P(fileNameNode)) {
		filename = Values::stringFromNode(fileNameNode);
		moduleKey = getModuleFileKey(filename, realFilename);
	} else for(std::vector<std::string>::const_iterator iterSearchPaths = searchPaths.begin(); iterSearchPaths != endSearchPaths; ++iterSearchPaths) {
		std::stringstream filenameStream;
		filenameStream << *iterSearchPaths;
		filenameStream << Values::stringFromNode(fileNameNode);
		filename = filenameStream.str();
		moduleKey = getModuleFileKey(filename, realFilename);
		if(moduleKey.length() != 0)
			break;
	}
	if(moduleKey.length() == 0 || realFilename.length() == 0) {
		std::string n = Values::stringFromNode(fileNameNode);
		fprintf(stderr, "could not find module \"%s\" in ", n.c_str());
		bool first = true;
		for (const auto& path : searchPaths) {
			if (!first)
				fprintf(stderr, ", ");
			first = false;
			fprintf(stderr, "\"%s\"", path.c_str());
		}
		std::cerr << std::endl;
		return(FALLBACK);
	}
	char* actualFilename = FFIs::get_absolute_path(realFilename.c_str());
	NodeT body = requireModule(actualFilename, moduleKey);
	return(Evaluators::reduce(Evaluators::uncurried(Evaluators::reduce(Evaluators::uncurried(&Module, body)), makeStr(actualFilename))));
}
DEFINE_FULL_OPERATION(ModuleLoader, {
	return(loadModule(fn, argument));
})
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

static inline NodeT makeModuleBox(NodeT argument) {
	return makeBox(argument, makeApplication(&ModuleBoxMaker, argument));
}
DEFINE_SIMPLE_STRICT_OPERATION(ModuleBoxMaker, makeModuleBox(argument))
REGISTER_BUILTIN(ModuleBoxMaker, 1, 0, symbolFromStr("makeModuleBox"))

Values::NodeT getModule(const char* filename) {
	return Values::makeApplication(&ModuleLoader, Values::makeStr(filename));
}
Values::NodeT getModuleEntryAccessor(const char* filename, Values::NodeT exportKey) {
	return Values::makeApplication(getModule(filename), Evaluators::quote(exportKey));
}

};
