#include <5D/Operations>
#include <5D/Evaluators>

#include "REPL/ExtREPL"
#include "REPL/Symbols"
namespace REPLX {
using namespace Values;
DECLARE_SIMPLE_OPERATION(RImporter)
DECLARE_SIMPLE_OPERATION(RInformant)
DECLARE_SIMPLE_OPERATION(RDefiner)
DECLARE_SIMPLE_OPERATION(ROperatorPrecedenceListGetter)
DECLARE_SIMPLE_OPERATION(RPurger)
DECLARE_SIMPLE_OPERATION(RExecutor)

static NodeT getMethod(NodeT name) {
	if(name == Symbols::SgetOperatorPrecedenceListexclam)
		return(&REPLX::ROperatorPrecedenceListGetter);
	//else if(name == Symbols::SgetEnvironment)
	//	return(&REnvironmentGetter);
	else if(name == Symbols::Sdescribeexclam)
		return(&REPLX::RInformant);
	else if(name == Symbols::Sdefineexcam)
		return(&REPLX::RDefiner);
	else if(name == Symbols::Simportexclam)
		return(&REPLX::RImporter);
	else if(name == Symbols::Spurgeexclam)
		return(&REPLX::RPurger);
	else if(name == Symbols::Sexecuteexclam)
		return(&REPLX::RExecutor);
	else if(name == Symbols::Sexports)
		return(makeCons(Symbols::SgetOperatorPrecedenceListexclam,
		       makeCons(Symbols::Sdescribeexclam,
		       makeCons(Symbols::Sdefineexcam,
		       makeCons(Symbols::Simportexclam,
		       makeCons(Symbols::Spurgeexclam,
		       makeCons(Symbols::Sexecuteexclam,
		       makeCons(Symbols::Sexports,
		       NULL))))))));
	else
		return(NULL); // TODO
}
DEFINE_SIMPLE_STRICT_OPERATION(REPLMethodGetter, getMethod(argument))
REGISTER_BUILTIN(REPLMethodGetter, 1, 0, symbolFromStr("REPLMethods"))

};
