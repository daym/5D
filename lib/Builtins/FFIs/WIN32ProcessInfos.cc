#include <vector>
#include "stdafx.h"
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "Evaluators/FFI"
//nclude "Evaluators/Builtins"
#include "FFIs/ProcessInfos"
#include <5D/Allocators>
#include <5D/Operations>
#include <5D/FFIs>

namespace FFIs {
using namespace Values;
static NodeT get_arch_dep_path(NodeT nameNode) {
	return(nameNode);
}
bool absolute_path_P(NodeT name) {
	if(name == NULL) // an empty path is not an absolute path.
		return(false);
	//if(name->size < 2)
	//	return(false);
	char* c = Values::stringFromNode(name);
	if(*c != '\\' && *c != '/' && *(c + 1) != ':')
		return(false);
	if(*c == '\\' || *c == '/')
		return(true);
	++c;
	if(*c == ':')
		return(true);
	return(false);
}
static NodeT internEnviron(WCHAR* envp) {
	if(*envp) {
		int count = wcslen(envp);
		NodeT head = makeStr(ToUTF8(envp));
		envp += count + 1;
		return(makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static NodeT wrapInternEnviron(NodeT argument) {
	// TODO check whether it worked? No.
	return internEnviron((WCHAR*) Values::pointerFromNode(argument));
}
static WCHAR* environFromListF(NodeT argument) {
	int count = 0;
	WCHAR* resultC;
	std::vector<std::wstring> result;
	int i = 0;
	NodeT listNode = Evaluators::reduce(argument);
	for(NodeT node = argument; !nil_P(node); node = get_cons_tail(node)) {
		std::wstring v = FromUTF8(stringFromNode(get_cons_head(node)));
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
	return(resultC);
}
BEGIN_PROC_WRAPPER(getEnviron, 0, symbolFromStr("getEnviron!"), )
	NodeT result;
	LPWSTR env = GetEnvironmentStringsW();
	result = MONADIC(internEnviron(env));
	FreeEnvironmentStringsW(env);
	return(result);
END_PROC_WRAPPER
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
static NodeT wrapGetAbsolutePath(NodeT options, NodeT argument) {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	char* text = iter->second ? Values::stringFromNode(iter->second) : NULL;
	FETCH_WORLD(iter);
	text = get_absolute_path(text);
	return(CHANGED_WORLD(makeStr(text)));
}
DEFINE_FULL_OPERATION(AbsolutePathGetter, {
	return(wrapGetAbsolutePath(fn, argument));
})
BEGIN_PROC_WRAPPER(listFromEnviron, 1, symbolFromStr("listFromEnviron!"), )
	NodeT env = FNARG_FETCH(node);
	return MONADIC(wrapInternEnviron(env));
END_PROC_WRAPPER
DEFINE_BOXED_STRICT_OPERATION(environFromList, environFromListF)

DEFINE_SIMPLE_STRICT_OPERATION(ArchDepLibNameGetter, get_arch_dep_path(argument))
DEFINE_SIMPLE_STRICT_OPERATION(AbsolutePathP, absolute_path_P(argument))

REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, symbolFromStr("absolutePath!"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, symbolFromStr("absolutePath?"))
REGISTER_BUILTIN(environFromList, 1, 0, symbolFromStr("environFromList"))

}; /* end namespace FFIs */

