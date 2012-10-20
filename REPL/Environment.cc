#include "Values/Values"
#include "REPL/Environment"

namespace REPL {

typedef AST::HashTable Environment;
static Environment* cloneEnvironment(Environment* original) {
	Environment* result = new Environment;
	if(original) {
		Environment::const_iterator end = original->end();
		for(const_iterator iter = original->begin(); iter != end; ++iter)
			(*result)[iter->first] = iter->second;
	}
	return(result);
}
Environment* addToEnvironment(Environment* orignal, AST::Symbol* key, AST::NodeT value) {
	Environment* result = cloneEnvironment(original);
	// FIXME what if key == NULL ?
	(*result)[key->name] = AST::makeCons(value, (*result)[key->name]);
	return(result);
}
AST::NodeT getFromEnvironment(Environment* environment, AST::Symbol* key, bool& B_found) {
	B_found = false;
	if(environment && key) {
		Environment::const_iterator iter = environment->find(key->name);
		if(iter != environment->end()) {
			B_found = true;
			return(AST::get_cons_head(iter->second));
		}
	}
	return(NULL);
}

}; /* end namespace */

