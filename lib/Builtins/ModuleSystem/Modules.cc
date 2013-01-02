#ifdef WIN32
#include "stdafx.h"
#endif
#include <stdio.h>
#include <5D/Operations>
#include <5D/Values>
#include <5D/FFIs>
#include "ModuleSystem/Modules"
#include "Evaluators/Evaluators"
#include "Values/Symbols"
#include "Evaluators/Builtins"
#include "Evaluators/ModuleLoader"
#include "Evaluators/BuiltinSelector"
#include "Formatters/SExpression"

/* note that a module can load other modules, hence this has a dependency to the ModuleLoader (only for this reason) */

namespace ModuleSystem {
using namespace Values;
using namespace Evaluators;
static __inline NodeT accessModule(NodeT fn, NodeT argument) {
	/* TODO do not automatically reduce? */
	NodeT body = get_curried_operation_argument(get_curried_operation_operation(fn));
	NodeT result = makeApplication(body, argument);
	result = Evaluators::reduce(result);
	return(result);
}

NodeT prepareModule(NodeT result, const char* filename) {
	Symbols::initSymbols();
	BuiltinSelector_init(); /* we have initialisation order problems otherwise (VERY hard to debug). */
	result = Evaluators::close(Symbols::Squote, &Evaluators::Quoter, result); // module needs that, so provide it.
	result = Evaluators::close(Symbols::Sdot, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, makeApplication(Symbols::Sa, Symbols::Sb))), result);
	result = Evaluators::close(Symbols::Shashexclam, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, Symbols::Sb)), result); // rem
	result = Evaluators::close(Symbols::Scolon, &Evaluators::Conser, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::Scomma, &Evaluators::Pairer, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::Snil, NULL, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::SrequireModule, &ModuleLoader, result); // module needs that, so provide it. // TODO maybe use Builtins.requireModule (not sure whether that's useful)
	result = Evaluators::close(Symbols::SBuiltins, &Evaluators::BuiltinGetter, result);
	if(filename)
		result = Evaluators::close(Symbols::Sfilename, Values::makeStr(filename), result);
	// ?? result = Evaluators::close(Symbols::SdispatchModule, makeAbstraction(Symbols::Sexports, makeApplication(&Evaluators::ModuleDispatcher, makeApplication(&Evaluators::ModuleBoxMaker, Symbols::Sexports))), result);
	result = Evaluators::provide_dynamic_builtins(result);
	result = Evaluators::annotate(result);
	return(result);
}
DEFINE_FULL_OPERATION(Module, {
	return(accessModule(fn, argument));
})
REGISTER_BUILTIN(Module, 3, 1, symbolFromStr("requireModule"));
static NodeT mapGetFst2(NodeT fallback, NodeT c) {
	if(nil_P(c)) {
		NodeT tail = fallback ? reduce(makeApplication(fallback, Symbols::Sexports)) : NULL;
		return(tail);
	} else {
		NodeT p = pairFromNode(get_cons_head(c));
		return(makeCons(get_pair_fst(p), mapGetFst2(fallback, get_cons_tail(c))));
	}
}
static NodeT accessHashtableF(NodeT box, NodeT key, NodeT junk) {
	Hashtable* table = (Hashtable*) pointerFromNode(box);
	const char* name = get_symbol1_name(key);
	if(name) {
		Hashtable::const_iterator iter = table->find(name);
		if(iter != table->end())
			return(iter->second);
	}
	return PREPARE(makeApplication((*table)["default"], key));
}
DEFINE_BINARY_STRICT2_OPERATION(HashtableAccessor, accessHashtableF)
REGISTER_BUILTIN(HashtableAccessor, 2, 0, symbolFromStr("FIXME"))

/* this takes care of caching the table in a Hashtable and adding an 'exports key listing all the keys. */
static NodeT dispatchModuleF(NodeT table, NodeT fallback, NodeT junk) {
	Symbols::initSymbols();
	Hashtable* m = new (UseGC) Hashtable;
	table = consFromNode(table);
	(*m)["exports"] = makeCons(Symbols::Sexports, mapGetFst2(fallback, table));
	for(; !nil_P(table); table = get_cons_tail(table)) {
		NodeT entry = pairFromNode(get_cons_head(table));
		std::string entryS = Evaluators::str(table);
		//printf("ENTRY %s\n", entryS.c_str());
		NodeT key = get_pair_fst(entry);
		NodeT value = get_pair_snd(entry);
		const char* name = get_symbol1_name(key);
		if(name && m->find(name) == m->end())
			(*m)[name] = value;
	}
	return Values::makeApplication(&HashtableAccessor, makeBox(m, NULL/*FIXME*/));
}
	
DEFINE_BINARY_STRICT2_OPERATION(ModuleDispatcher, dispatchModuleF)
REGISTER_BUILTIN(ModuleDispatcher, 2, 0, symbolFromStr("dispatchModule"))
};
