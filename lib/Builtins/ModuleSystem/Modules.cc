#ifdef WIN32
#include "stdafx.h"
#endif
#include <5D/Operations>
#include <5D/Values>
#include <5D/FFIs>
#include "ModuleSystem/Modules"
#include "Evaluators/Evaluators"
#include "Values/Symbols"
#include "Evaluators/Builtins"
#include "Evaluators/ModuleLoader"
#include "Evaluators/BuiltinSelector"

/* note that a module can load other modules, hence this has a dependency to the ModuleLoader (only for this reason) */

namespace ModuleSystem {
using namespace Values;
using namespace Evaluators;
static __inline NodeT access_module(NodeT fn, NodeT argument) {
	NodeT body = get_curried_operation_argument(get_curried_operation_operation(fn));
	NodeT result = Evaluators::reduce(makeApplication(body, argument));
	return(result);
}

NodeT prepareModule(NodeT result) {
	Symbols::initSymbols();
	result = Evaluators::close(Symbols::Squote, &Evaluators::Quoter, result); // module needs that, so provide it.
	result = Evaluators::close(Symbols::Sdot, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, makeApplication(Symbols::Sa, Symbols::Sb))), result);
	result = Evaluators::close(Symbols::Shashexclam, makeAbstraction(Symbols::Sa, makeAbstraction(Symbols::Sb, Symbols::Sb)), result); // rem
	result = Evaluators::close(Symbols::Scolon, &Evaluators::Conser, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::Scomma, &Evaluators::Pairer, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::Snil, NULL, result); // dispatch [] needs that, so provide it.
	result = Evaluators::close(Symbols::SrequireModule, &ModuleLoader, result); // module needs that, so provide it. // TODO maybe use Builtins.requireModule (not sure whether that's useful)
	result = Evaluators::close(Symbols::SBuiltins, &Evaluators::BuiltinGetter, result);
	// ?? result = Evaluators::close(Symbols::SdispatchModule, makeAbstraction(Symbols::Sexports, makeApplication(&Evaluators::ModuleDispatcher, makeApplication(&Evaluators::ModuleBoxMaker, Symbols::Sexports))), result);
	result = Evaluators::provide_dynamic_builtins(result);
	result = Evaluators::annotate(result);
	return(result);
}
DEFINE_FULL_OPERATION(Module, {
	return(access_module(fn, argument));
})
REGISTER_BUILTIN(Module, 3, 1, symbolFromStr("requireModule"));
static NodeT mapGetFst2(NodeT fallback, Cons* c) {
	if(c == NULL) {
		NodeT tail = fallback ? reduce(makeApplication(fallback, Symbols::Sexports)) : NULL;
		return(tail);
	} else
		return(makeCons(Evaluators::get_pair_first(reduce(evaluateToCons(reduce(get_cons_head(c))))), mapGetFst2(fallback, evaluateToCons(get_cons_tail(c)))));
}

static NodeT dispatchModule(NodeT options, NodeT argument) {
	/* parameters: <exports> <fallback> <key> 
	               2         1          0*/
	CXXArguments arguments = Evaluators::CXXfromArgumentsU(options, argument, 1);
	CXXArguments::const_iterator iter = arguments.begin();
	/*std::stringstream buffer;
	std::string v;
	int position = 0;
	Formatters::Math::print_CXX(new Scanners::OperatorPrecedenceList(), buffer, position, iter->second, 0, false);
	v = buffer.str();
	printf("%s\n", v.c_str());*/
	Box* mBox = dynamic_cast<Box*>(iter->second);
	++iter;
	NodeT fallback = iter->second;
	++iter;
	NodeT key = iter->second;
	HashTable* m;
	if(!mBox) {
		throw Evaluators::EvaluationException("invalid symbol table entry (*)");
		return(NULL);
	}
	void* pBox = Values::pointerFromNode(mBox);
	if(dynamic_cast<HashTable*>((NodeT) pBox) == NULL) {
		m = new (UseGC) HashTable;
		for(Cons* table = (Cons*) pBox; table; table = Evaluators::evaluateToCons(table->tail)) {
			std::string irv = Evaluators::str(table->head);
			//printf("irv %s\n", irv.c_str());
			Cons* entry = evaluateToCons(reduce(get_cons_head(table)));
			//std::string v = str(entry);
			//printf("=irv> %s\n", v.c_str());
			NodeT x_key = reduce(Evaluators::get_pair_first(entry));
			//std::string vkey = Evaluators::str(x_key);
			//printf("=key> %s\n", vkey.c_str());
			NodeT value = reduce(Evaluators::get_pair_second(entry));
			const char* name = get_symbol1_name(x_key);
			if(name && m->find(name) == m->end())
				(*m)[name] = value;
		}
		Cons* table = (Cons*) pBox;
		set_box_value(mBox, m);
		(*m)["exports"] = makeCons(Symbols::Sexports, mapGetFst2(fallback, table));
	}
	m = (HashTable*) mBox->value;
	//std::string vkey = Evaluators::str(key);
	//printf("%s\n", vkey.c_str());
	const char* name = get_symbol_name(key); 
	if(name) {
		/*HashTable::const_iterator b = m->begin();
		HashTable::const_iterator e = m->end();
		for(; b != e; ++b) {
			printf("%s<\n", b->first);
		}
		printf("searching \"%s\"\n", s->name);*/
		HashTable::const_iterator iter = m->find(name);
		if(iter != m->end())
			return(iter->second);
		else {
			if(fallback) { // this should always hold
				return(reduce(makeApplication(fallback, key)));
			} else { // this is a leftover
				std::stringstream sst;
				sst << "library does not contain '" << name;
				std::string v = sst.str();
				throw Evaluators::EvaluationException(GCx_strdup(v.c_str()));
			}
		}
	} else
		throw Evaluators::EvaluationException("not a symbol");
	return(NULL);
}
DEFINE_FULL_OPERATION2(ModuleDispatcher, dispatchModule)
REGISTER_BUILTIN(ModuleDispatcher, (-3), 1, symbolFromStr("dispatchModule"))
};
