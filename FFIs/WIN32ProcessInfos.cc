#include <vector>
#include "stdafx.h"
#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
#include "FFIs/ProcessInfos"
#include "FFIs/Allocators"

namespace FFIs {
static AST::NodeT get_arch_dep_path(AST::NodeT nameNode) {
	return(nameNode);
}
bool absolute_path_P(AST::NodeT name) {
	if(name == NULL) // an empty path is not an absolute path.
		return(false);
	//if(name->size < 2)
	//	return(false);
	char* c = Evaluators::get_string(name);
	if(*c != '\\' && *c != '/' && *(c + 1) != ':')
		return(false);
	if(*c == '\\' || *c == '/')
		return(true);
	++c;
	if(*c == ':')
		return(true);
	return(false);
}
static AST::NodeT internEnviron(WCHAR* envp) {
	if(*envp) {
		int count = wcslen(envp);
		AST::NodeT head = AST::makeStr(ToUTF8(envp));
		envp += count + 1;
		return(AST::makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static AST::NodeT wrapInternEnviron(AST::NodeT argument) {
	// TODO check whether it worked? No.
	return internEnviron((WCHAR*) Evaluators::get_pointer(argument));
}
static AST::Box* environFromList(AST::NodeT argument) {
	int count = 0;
	WCHAR* resultC;
	std::vector<std::wstring> result;
	int i = 0;
	AST::NodeT listNode = Evaluators::reduce(argument);
	for(AST::Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		std::wstring v = FromUTF8(Evaluators::get_string(node->head));
		result.push_back(v);
		count += v.length() + 1;
		// FIXME handle overflow
	}
	resultC = (WCHAR*) GC_MALLOC(sizeof(WCHAR) * (count + 1));
	std::vector<std::wstring>::const_iterator endIter = result.end();
	i = 0;
	for(std::vector<std::wstring>::const_iterator iter = result.begin(); iter != endIter; ++iter) {
		std::wstring v = *iter;
		memcpy(&resultC[i], v.c_str(), (v.length() + 1) * sizeof(WCHAR));
		i += v.length() + 1;
	}
	resultC[i] = 0;
	return(AST::makeBox(resultC, AST::makeApplication(&EnvironFromList, listNode/* or argument*/)));
}
static AST::NodeT wrapGetEnviron(AST::NodeT world) {
	AST::NodeT result;
	LPWSTR env = GetEnvironmentStringsW();
	result = CHANGED_WORLD(internEnviron(env));
	FreeEnvironmentStringsW(env);
	return(result);
}
DEFINE_SIMPLE_OPERATION(EnvironGetter, wrapGetEnviron(Evaluators::reduce(argument)))
char* get_absolute_path(const char* filename) {
	if(filename == NULL || filename[0] == 0)
		filename = ".";
	std::wstring filenameW = FromUTF8(filename);
	WCHAR buffer[2049];
	if(GetFullPathNameW(filenameW.c_str(), 2048, buffer, NULL) != 0) {
		return(ToUTF8(buffer));
	} else
		return(GCx_strdup(filename));
}
static AST::NodeT wrapGetAbsolutePath(AST::NodeT options, AST::NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	char* text = iter->second ? Evaluators::get_string(iter->second) : NULL;
	FETCH_WORLD(iter);
	text = get_absolute_path(text);
	return(CHANGED_WORLD(AST::makeStr(text)));
}
DEFINE_FULL_OPERATION(AbsolutePathGetter, {
	return(wrapGetAbsolutePath(fn, argument));
})
DEFINE_SIMPLE_OPERATION(EnvironInterner, wrapInternEnviron(Evaluators::reduce(argument)))
DEFINE_SIMPLE_OPERATION(EnvironFromList, environFromList(argument))

DEFINE_SIMPLE_OPERATION(ArchDepLibNameGetter, get_arch_dep_path(Evaluators::reduce(argument)))
DEFINE_SIMPLE_OPERATION(AbsolutePathP, absolute_path_P(Evaluators::reduce(argument)))

REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, AST::symbolFromStr("absolutePath!"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, AST::symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, AST::symbolFromStr("absolutePath?"))
REGISTER_BUILTIN(EnvironGetter, 1, 0, AST::symbolFromStr("environ!"))
REGISTER_BUILTIN(EnvironInterner, 1, 0, AST::symbolFromStr("listFromEnviron"))
REGISTER_BUILTIN(EnvironFromList, 1, 0, AST::symbolFromStr("environFromList"))

}; /* end namespace FFIs */

