#include <map>
#include <string>
#include <string.h>
#include "AST/Symbol"

namespace AST {

static std::map<std::string, Symbol*> symbols;

Symbol* intern(const char* name) {
	std::map<std::string, Symbol*>::const_iterator iter = symbols.find(name);
	if(iter != symbols.end()) {
		return(iter->second);
	} else {
		Symbol* result = new Symbol;
		result->name = strdup(name);
		symbols[name] = result;
		return(result);
	}
}

}; /* end namespace AST */
