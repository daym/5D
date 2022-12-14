/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <map>
#include <string>
#include <string.h>
#include "Values/Values"
#include <5D/Allocators>

namespace Values {

static Hashtable* keywords;

/* TODO we can also just skip the whole map business for single-character names if we just return the character code instead of fumbling around (would have to make sure actual addresses are >255 then). 
   of course, str would then have to be global and we can't use the VMT anymore. Not sure whether it would be worth it. */
NodeT keywordFromStr(const char* name) {
	if(keywords == NULL) {
		GC_INIT();
		keywords = new Hashtable;
	}
	Hashtable::const_iterator iter = keywords->find(name);
	if(iter != keywords->end()) {
		return((Keyword*) iter->second);
	} else {
		Keyword* result = new Keyword;
		result->name = GCx_strdup(name);
		(*keywords)[result->name] = result;
		return(result);
	}
}

}; /* end namespace Values */
