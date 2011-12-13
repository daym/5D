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
#include "AST/Symbol"
#include "AST/Symbols"
#include "Scanners/OperatorPrecedenceList"

namespace Scanners {
using namespace AST;

// TODO REGISTER_STR(OperatorPrecedenceList, return(...);)
// TODO REGISTER_STR(OperatorPrecedenceItem, return(...);)
int OperatorPrecedenceList::get_operator_precedence_and_associativity(AST::Symbol* symbol, AST::Symbol*& associativity_out) {
	if(symbol != NULL)
		for(int i = 0; i < MAX_PRECEDENCE_LEVELS; ++i) {
			for(struct OperatorPrecedenceItem* item = levels[i]; item; item = item->next)
				if(item->operator_ == symbol) {
					associativity_out = item->associativity;
					return(i);
				}
		}
	associativity_out = NULL;
	return(-1);
}
int OperatorPrecedenceList::get_operator_precedence(AST::Symbol* symbol) {
	AST::Symbol* assoc;
	return(get_operator_precedence_and_associativity(symbol, assoc));
}
void OperatorPrecedenceList::cons(int precedence_level, struct AST::Symbol* operator_, struct AST::Symbol* associativity) {
	assert(precedence_level >= 0 && precedence_level < MAX_PRECEDENCE_LEVELS);
	if(prefix_usages[(unsigned char) operator_->name[0]] > 0) { /* someone has something like this in use, maybe hapless caller */
		int level = get_operator_precedence(operator_);
		while(level != -1) {
			fprintf(stderr, "warning: the same operator \"%s\" previously had precedence level %d, defusing it.\n", operator_->name, level);
			for(struct OperatorPrecedenceItem* item = levels[level]; item; item = item->next)
				if(item->operator_ == operator_)
					item->operator_ = NULL; // XXX
			level = get_operator_precedence(operator_);
		}
	}
	++prefix_usages[(unsigned char) operator_->name[0]];
	levels[precedence_level] = new OperatorPrecedenceItem(levels[precedence_level], operator_, associativity);
#ifndef SIMPLE_APPLICATION
	if(operator_ == symbolFromStr("+"))
		apply_level = next_precedence_level(precedence_level);
#endif
	if(operator_ == symbolFromStr("-"))
		minus_level = precedence_level;
}
void OperatorPrecedenceList::uncons(int precedence_level, struct AST::Symbol* operator_, struct AST::Symbol* associativity) {
	struct OperatorPrecedenceItem* item;
	assert(precedence_level >= 0 && precedence_level < MAX_PRECEDENCE_LEVELS);
	assert(levels[precedence_level] && levels[precedence_level]->operator_ == operator_);
	--prefix_usages[(unsigned char) operator_->name[0]];
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
#ifdef SIMPLE_APPLICATION
	apply_level = 13;
	cons(apply_level, I(" "), L); // apply
#endif
	if(bInitDefaults) {
		cons(19, I("_"), R);
		cons(19, I("."), L);
		cons(19, I("^"), R);
		cons(17, I("**"), R);
		cons(16, I("⨯"), R);
		cons(15, I("*"), L);
		cons(15, I("/"), L);
		// TODO div rem quot 17 L
		cons(14, I(":"), R);
		cons(12, I("++"), L);
		cons(11, I("+"), L);
		cons(11, I("-"), L);
		cons(10, I("%"), L);
		cons(9, I("="), N);
		cons(9, I("/="), N);
		cons(8, I("<"), N);
		cons(8, I("<="), N);
		cons(8, I(">"), N);
		cons(8, I(">="), N);
		cons(8, I("≤"), N);
		cons(8, I("≥"), N);
		cons(7, I("&&"), L);
		cons(6, I("||"), L);
		cons(5, I(","), L), // FIXME R
		cons(4, I("$"), R);
		//cons(?, I(">>"), L);
		//cons(?, I(">>="), L);
		//cons(?, I("$"), L);
		cons(3, I("=>"), L); // FIXME precedence.
		cons(2, I("|"), L);
		cons(1, I(";"), L);
		// cons(0, I("\\"), P);
		// TODO '(', 'auto('
	}
#undef N
#undef L
#undef R
#undef I
}

}; /* end namespace Scanners */
