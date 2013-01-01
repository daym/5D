#include <stdarg.h>
#include <stdio.h>
#include <5D/Allocators>
#include <5D/Values>
#include <5D/Operations>
#include "Values/Values"
#include "Values/Symbols"
/* TODO clean up */
#include "Evaluators/Evaluators"
#include "Evaluators/Builtins"

namespace ModuleSystem {
using namespace Values;
using namespace Symbols;

static NodeT makeEntry(NodeT name, void* fn) {
	NodeT entry = makePair(name, makeBox(fn, name/*TODO replicate the entire accessor */));
	return(entry);
}
static NodeT makeExportsFVA(const char* fmt, char* names, va_list ap) {
	char buf[2049];
	while(*names && (*names == ' ' || *names == '&'))
		++names;
	if(*names && *names != ',') {
		char* x;
		NodeT name;
		x = strchr(names, ',');
		if(!x)
			x = names + strlen(names);
		else
			*x = 0;
		if(snprintf(buf, 2048, fmt, names) == -1)
			abort();
		name = symbolFromStr(buf); // makeStr(buf);
		names = x;
		void* fn = va_arg(ap, void*);
		return(makeCons(makeEntry(name, fn), makeExportsFVA(fmt, names, ap)));
	} else
		return(NULL);
}
/*NodeT makeReflector(NodeT entries) {
	NodeT entry = get_cons_head(entries);
	NodeT name = get_pair_fst(entry);
	return(makeCons(name, makeReflector(get_cons_tail(entries))));
}*/
NodeT makeExportsQ(const char* names, ...) {
	NodeT result;
	va_list ap;
	va_start(ap, names);
	result = makeExportsFVA("%s", GCx_strdup(names), ap);
	va_end(ap);
	//result = makeCons(makePair(Sexports, makeCons(Sexports, makeReflector(result))), result); // exports are automatcially added by ModuleSystem dispatcher
	return(result);
}
NodeT makeExportsFQ(const char* fmt, const char* names, ...) {
	NodeT result;
	va_list ap;
	va_start(ap, names);
	result = makeExportsFVA(fmt, GCx_strdup(names), ap);
	va_end(ap);
	//result = makeCons(makePair(Sexports, makeCons(Sexports, makeReflector(result))), result); // exports are automatcially added by ModuleSystem dispatcher
	return(result);
}

/* and the entire thing in the language itself: */

static NodeT buildHashExports(NodeT node) {
	if(nil_P(node))
		return(node);
	NodeT head = get_cons_head(node);
	NodeT tail = Evaluators::evaluateToCons(get_cons_tail(node));
	NodeT result = makeApplication(makeApplication(&Evaluators::Pairer, makeApplication(&Evaluators::Quoter, head)), head);
	return makeCons(result, buildHashExports(tail));
}
/*
better:

parseExports! = (\world
	parseValue! world ;\value
	return! (map (\item ((quote item), item)) (eval value [('(:), (:)) ('nil, nil) ('(,), (,)]))
) in 
*/

DEFINE_SIMPLE_STRICT_OPERATION(HashExporter, buildHashExports(argument))
REGISTER_BUILTIN(HashExporter, 1, 0, symbolFromStr("#exports"))

};
