/*
4D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <string.h>
#include "Evaluators/FFI"
#include "AST/AST"

namespace Evaluators {
using namespace AST;

int get_native_integer(AST::Node* root) {
	/* FIXME */
	return(0);
}
void* get_native_pointer(AST::Node* root) {
	/* FIXME */
	return(NULL);
}
bool get_native_boolean(AST::Node* root) {
	/* FIXME */
	return(false);
}
char* get_native_string(AST::Node* root) {
	AST::String* rootString = dynamic_cast<AST::String*>(root);
	if(rootString)
		return(strdup(rootString->text));
	else {
		std::string value = root->str(); /* FIXME */
		return(strdup(value.c_str()));
	}
}
AST::Node* FFI::execute(AST::Node* argument) {
	//lambda state: makeList(executeLowlevel(argument), state);
	// TODO maybe cache that, makes not a lot of sense to regenerate the intermediate things all the time!
	if(B_pure)
		return(executeLowlevel(argument));
	else
		return(new FFIClosure(argument, this));
}
AST::Node* FFIClosure::execute(AST::Node* state) {
	/* TODO change state if neccessary. Cache the old result? */
	if(routine)
		return(cons(routine->executeLowlevel(argument), cons(state, NULL)));
	else
		return(NULL);
}
AST::Node* SymArgCacheFFI::executeLowlevel(AST::Node* argument) {
	/* TODO for non-pure, this doesn't make a whole lot of sense. */
/*	AST::Symbol* argumentSymbol = dynamic_cast<AST::Symbol*>(argument);
	if(argumentSymbol == NULL)
		return(NULL);
	std::map<AST::Symbol*, AST::Node*>::const_iterator iter = knownResults.find(argumentSymbol);
	if(iter != knownResults.end())
		return(iter->second);
	knownResults[argumentSymbol] = executeUncached(argumentSymbol);
	return(knownResults[argumentSymbol]);
	*/
	return(NULL); /* TODO */
}

}; /* end namspace */
