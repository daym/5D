#include <sstream>
#include <vector>
#include <sys/utsname.h>
#include "Values/Values"
#include "Evaluators/Evaluators"
#include "FFIs/ProcessInfos"
#include "FFIs/Allocators"
#include "Evaluators/Builtins"
#include "Evaluators/FFI"
#include "Evaluators/Operation"

namespace FFIs {

static AST::Str* get_arch_dep_path(AST::NodeT nameNode) {
	if(nameNode == NULL)
		return(NULL);
	const char* name = Evaluators::get_string(nameNode);
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
	return(AST::makeStrCXX(sst.str()));
}
bool absolute_path_P(AST::NodeT nameN) {
	char* c = nameN ? Evaluators::get_string(nameN) : NULL;
	if(!c || !*c)
		return(false);
	return(*c == '/');
}
static AST::NodeT internEnviron(const char** envp) {
	if(*envp) {
		AST::NodeT head = AST::makeStr(*envp++);
		return(AST::makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static AST::NodeT wrapInternEnviron(AST::NodeT argument) {
	return internEnviron((const char**) Evaluators::get_pointer(argument));
}
static AST::Box* environFromList(AST::NodeT argument) {
	int count = 0;
	char** result;
	int i = 0;
	AST::NodeT listNode = Evaluators::reduce(argument);
	for(AST::Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		++count;
		// FIXME handle overflow
	}
	result = (char**) GC_MALLOC(sizeof(char*) * (count + 1));
	for(AST::Cons* node = Evaluators::evaluateToCons(listNode); node; node = Evaluators::evaluateToCons(node->tail)) {
		result[i] = Evaluators::get_string(node->head);
		++i;
	}
	return(AST::makeBox(result, AST::makeApplication(&EnvironFromList, listNode/* or argument*/)));
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

