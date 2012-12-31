#include <sstream>
#include <vector>
#include <sys/utsname.h>
#include <5D/Allocators>
#include <5D/Operations>
#include <5D/FFIs>
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "FFIs/ProcessInfos"
#include "Evaluators/Builtins"
#include "Evaluators/FFI"

namespace FFIs {
using namespace Values;

static NodeT get_arch_dep_path(NodeT nameNode) {
	if(nameNode == NULL)
		return(NULL);
	const char* name = Values::stringFromNode(nameNode);
	// keep that result constant and invariant.
	std::stringstream sst;
	//std::string name((char*) nameNode->native, nameNode->size);
	sst << "/lib/";
	struct utsname buf;
	if(uname(&buf) == -1)
		sst << "x86_64";
	else
		sst << buf.machine;
	sst << "-linux-gnu/";
	sst << name;
	return(makeStrCXX(sst.str()));
}
bool absolute_path_P(NodeT nameN) {
	char* c = nameN ? Values::stringFromNode(nameN) : NULL;
	if(!c || !*c)
		return(false);
	return(*c == '/');
}
static NodeT internEnviron(const char** envp) {
	if(*envp) {
		NodeT head = makeStr(*envp++);
		return(makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static NodeT wrapInternEnviron(NodeT argument) {
	return internEnviron((const char**) Values::pointerFromNode(argument));
}
static Box* environFromList(NodeT argument) {
	int count = 0;
	char** result;
	int i = 0;
	NodeT listNode = argument;
	for(Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		++count;
		// FIXME handle overflow
	}
	result = (char**) GC_MALLOC(sizeof(char*) * (count + 1));
	for(Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		result[i] = Values::stringFromNode(node->head);
		++i;
	}
	return(makeBox(result, makeApplication(&EnvironFromList, listNode/* or argument*/)));
}
DEFINE_FULL_OPERATION(EnvironGetter, {
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(fn, argument);
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	FETCH_WORLD(iter);
	return(CHANGED_WORLD(internEnviron((const char**) environ)));
})
char* get_absolute_path(const char* filename) {
	if(filename && filename[0] == '/')
		return(GCx_strdup(filename));
	else {
		char buffer[2049];
		std::stringstream sst;
		if(getcwd(buffer, 2048)) {
			sst << buffer;
			if(buffer[0] && buffer[strlen(buffer) - 1] != '/')
				sst << '/';
		}
		if(filename)
			sst << filename;
		std::string v = sst.str();
		return(GCx_strdup(v.c_str()));
	}
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
DEFINE_SIMPLE_STRICT_OPERATION(EnvironInterner, wrapInternEnviron(argument))
DEFINE_SIMPLE_STRICT_OPERATION(EnvironFromList, environFromList(argument))

DEFINE_SIMPLE_STRICT_OPERATION(ArchDepLibNameGetter, get_arch_dep_path(argument))
DEFINE_SIMPLE_STRICT_OPERATION(AbsolutePathP, absolute_path_P(argument))

REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, symbolFromStr("absolutePath!"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, symbolFromStr("absolutePath?"))
REGISTER_BUILTIN(EnvironGetter, 1, 0, symbolFromStr("environ!"))
REGISTER_BUILTIN(EnvironInterner, 1, 0, symbolFromStr("listFromEnviron"))
REGISTER_BUILTIN(EnvironFromList, 1, 0, symbolFromStr("environFromList"))

}; /* end namespace FFIs */

