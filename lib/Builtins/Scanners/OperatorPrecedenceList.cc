/*
5D programming language
Copyright (C) 2011  Danny Milosavljevic
This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "Values/Values"
#include "Values/Symbols"
#include "Scanners/OperatorPrecedenceList"
#include <5D/Evaluators>

namespace Scanners {
using namespace Values;

// NO REGISTER_STR(OperatorPrecedenceList, return("operatorPrecedenceList");)
// TODO REGISTER_STR(OperatorPrecedenceItem, return(...);)
int OperatorPrecedenceList::get_operator_precedence_and_associativity(NodeT symbol, NodeT& associativity_out) const {
	if(symbol != NULL) {
		//std::string opname = Evaluators::str(symbol);
		//printf("%s\n", opname.c_str());
		for(int i = 0; i < MAX_PRECEDENCE_LEVELS; ++i) {
			for(struct OperatorPrecedenceItem* item = levels[i]; item; item = item->next)
				if(item->operator_ == symbol) {
					associativity_out = item->associativity;
					return(i);
				}
		}
	}
	associativity_out = NULL;
	return(-1);
}
int OperatorPrecedenceList::get_operator_precedence(NodeT symbol) const {
	NodeT assoc;
	return(get_operator_precedence_and_associativity(symbol, assoc));
}
void OperatorPrecedenceList::cons(int precedence_level, NodeT operator_, NodeT associativity) {
	assert(precedence_level >= 0 && precedence_level < MAX_PRECEDENCE_LEVELS);
	if(operator_ == NULL) {
		operator_ = symbolFromStr(" "); /* apply */
	}
	if(operator_ == symbolFromStr(" ")) {
	        apply_level = precedence_level; /* there should be no other things at the same level */
	}
	std::string opname = Evaluators::str(operator_);
	if(prefix_usages[(unsigned char) opname.c_str()[0]] > 0) { /* someone has something like this in use, maybe hapless caller */
		int level = get_operator_precedence(operator_);
		while(level != -1) {
			fprintf(stderr, "warning: the same operator \"%s\" previously had precedence level %d, defusing it.\n", opname.c_str(), level);
			for(struct OperatorPrecedenceItem* item = levels[level]; item; item = item->next)
				if(item->operator_ == operator_)
					item->operator_ = NULL; // XXX
			level = get_operator_precedence(operator_);
		}
	}
	++prefix_usages[(unsigned char) opname.c_str()[0]];
	levels[precedence_level] = new OperatorPrecedenceItem(levels[precedence_level], operator_, associativity);
	if(operator_ == symbolFromStr("-"))
		minus_level = precedence_level;
}
void OperatorPrecedenceList::uncons(int precedence_level, NodeT operator_, NodeT associativity) {
	struct OperatorPrecedenceItem* item;
	assert(precedence_level >= 0 && precedence_level < MAX_PRECEDENCE_LEVELS);
	assert(levels[precedence_level] && levels[precedence_level]->operator_ == operator_);
	std::string opname = Evaluators::str(operator_);
	--prefix_usages[(unsigned char) opname.c_str()[0]];
	item = levels[precedence_level]->next;
	levels[precedence_level]->next = NULL;
	levels[precedence_level] = item;
}
OperatorPrecedenceList::OperatorPrecedenceList(bool bInitDefaults) {
	apply_level = 0;
	for(int i = 0; i < MAX_PRECEDENCE_LEVELS; ++i)
		levels[i] = NULL;
	for(int i = 0; i < 256; ++i)
		prefix_usages[i] = 0;
#define I(x) symbolFromStr(x)
#define R Symbols::Sright
#define L Symbols::Sleft
#define N Symbols::Snone
#define P Symbols::Sprefix
#define S Symbols::Spostfix
	apply_level = 24;
	cons(apply_level, I(" "), L); // apply
	if(bInitDefaults) {
		cons(32, I("."), L); // like quote
		cons(31, I("_"), R);
		cons(31, I("^"), R);
		cons(30, I("**"), R);
		cons(29, I("*"), L);
		cons(29, I("⋅"), L);
		cons(29, I("/"), L);
		cons(28, I("⨯"), R); /* TODO also for sets */
		cons(27, I(":"), R);
		cons(26, I("'"), P);
		cons(23, I("++"), L);
		cons(22, I("+"), L);
		cons(22, I("unary-"), P);
		cons(22, I("-"), L);  /* also set difference. TODO maybe use the special char "−" for that. */
		cons(21, I("%"), L);
		/* TODO set-excludor (\\) */
		cons(16, I("∩"), L);
		cons(15, I("∪"), L);
		cons(14, I("∈"), N);
		cons(14, I("⊂"), N);
		cons(14, I("⊃"), N); /* could also mean "implies", then it would be right-associative */
		cons(14, I("⊆"), N);
		cons(14, I("⊇"), N);
		cons(10, I("="), N);
		cons(10, I("≟"), N);
		cons(10, I("/="), N);
		cons(9, I("<"), N);
		cons(9, I("<="), N);
		cons(9, I(">"), N);
		cons(9, I(">="), N);
		cons(9, I("≤"), N);
		cons(9, I("≥"), N);
		cons(8, I("&&"), L);
		cons(8, I("∧"), L);
		cons(7, I("||"), L);
		cons(7, I("∨"), L);
		cons(6, I(","), R);
		cons(5, I("$"), R);
		//cons(4, I("if"), R);
		cons(4, I("elif"), R);
		cons(4, I("else"), R);
		//cons(?, I(">>"), L);
		//cons(?, I(">>="), L);
		cons(3, I("|"), L);
		cons(2, I("=>"), L);
		cons(2, I(";"), L);
		cons(2, I("?;"), L);
		cons(1, I("\\"), P);
		//cons(1, I("→"), R); // mapsto
		cons(0, I("let"), R); // and technically define, defrec, def
		cons(0, I("let!"), R);
		cons(0, I("import"), R);
		// TODO implies: RIGHT associative.
	}
#undef S
#undef P
#undef N
#undef L
#undef R
#undef I
}
static NodeT get_level_operators(struct OperatorPrecedenceItem* item) {
	return item ? makeCons(item->operator_, get_level_operators(item->next)) : NULL;
}
NodeT OperatorPrecedenceList::get_all_operators(int precedence_level) const {
	if(precedence_level >= 0 && precedence_level < MAX_PRECEDENCE_LEVELS) {
		NodeT h = get_level_operators(levels[precedence_level]);
		return(makeCons(h, get_all_operators(next_precedence_level(precedence_level))));
	} else
		return(NULL);
}

}; /* end namespace Scanners */
