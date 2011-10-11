/*
5D vector analysis program
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <map>
#include <string>
#include <string.h>
#include "AST/Keyword"

namespace AST {

static std::map<std::string, Keyword*>* keywords;

/* TODO we can also just skip the whole map business for single-character names if we just return the character code instead of fumbling around (would have to make sure actual addresses are >255 then). 
   of course, str would then have to be global and we can't use the VMT anymore. Not sure whether it would be worth it. */
Keyword* keywordFromString(const char* name) {
	if(keywords == NULL)
		keywords = new std::map<std::string, Keyword*>();
	std::string xname = name;
	std::map<std::string, Keyword*>::const_iterator iter = keywords->find(xname);
	if(iter != keywords->end()) {
		return(iter->second);
	} else {
		Keyword* result = new Keyword;
		result->name = strdup(name);
		(*keywords)[name] = result;
		return(result);
	}
}

std::string Keyword::str(void) const {
	return std::string("@") + (name);
}

}; /* end namespace AST */
