#include "AST/AST"
#include "Evaluators/Evaluators"
#include "FFIs/ProcessInfos"
#include "FFIs/Allocators"

namespace FFIs {

static AST::Str* get_arch_dep_path(AST::Str* nameNode) {
	if(nameNode == NULL)
		return(NULL);
	// keep that result constant and invariant.
	std::stringstream sst;
	std::string name((char*) nameNode->native, nameNode->size);
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
bool absolute_path_P(AST::Str* name) {
	if(name == NULL) // an empty path is not an absolute path.
		return(false);
	if(name->size < 1)
		return(false);
	char* c = (char*) name->native;
	return(*c == '/');
}
static AST::Node* internEnviron(const char** envp) {
	if(*envp) {
		AST::Node* head = AST::makeStr(*envp++);
		return(AST::makeCons(head, internEnviron(envp)));
	}
	else
		return(NULL);
}
static AST::Node* wrapInternEnviron(AST::Node* argument) {
	AST::Box* envp = dynamic_cast<AST::Box*>(argument);
	// TODO check whether it worked? No.
	return internEnviron((const char**) envp->native);
}
static AST::Box* environFromList(AST::Node* argument) {
	int count = 0;
	char** result;
	int i = 0;
	AST::Node* listNode = reduce(argument);
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
DEFINE_SIMPLE_OPERATION(EnvironGetter, Evaluators::makeIOMonad(internEnviron((const char**) environ), reduce(argument)))
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
static AST::Node* wrapGetAbsolutePath(AST::Node* options, AST::Node* argument) {
	CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	CXXArguments::const_iterator iter = arguments.begin();
	char* text = iter->second ? get_string(iter->second) : NULL;
	++iter;
	AST::Node* world = iter->second;
	text = get_absolute_path(text);
	return(Evaluators::makeIOMonad(AST::makeStr(text), world));
}
DEFINE_FULL_OPERATION(AbsolutePathGetter, {
	return(wrapGetAbsolutePath(fn, argument));
})
DEFINE_SIMPLE_OPERATION(EnvironInterner, wrapInternEnviron(reduce(argument)))
DEFINE_SIMPLE_OPERATION(EnvironFromList, environFromList(argument))

DEFINE_SIMPLE_OPERATION(ArchDepLibNameGetter, get_arch_dep_path(dynamic_cast<AST::Str*>(reduce(argument))))
DEFINE_SIMPLE_OPERATION(AbsolutePathP, absolute_path_P(dynamic_cast<AST::Str*>(reduce(argument))))

REGISTER_BUILTIN(AbsolutePathGetter, 2, 0, AST::symbolFromStr("absolutePath!"))
REGISTER_BUILTIN(ArchDepLibNameGetter, 1, 0, AST::symbolFromStr("archDepLibName"))
REGISTER_BUILTIN(AbsolutePathP, 1, 0, AST::symbolFromStr("absolutePath?"))
REGISTER_BUILTIN(ErrnoGetter, 1, 0, AST::symbolFromStr("errno!"))
REGISTER_BUILTIN(EnvironGetter, 1, 0, AST::symbolFromStr("environ!"))
REGISTER_BUILTIN(EnvironInterner, 1, 0, AST::symbolFromStr("listFromEnviron"))
REGISTER_BUILTIN(EnvironFromList, 1, 0, AST::symbolFromStr("environFromList"))

}; /* end namespace FFIs */

