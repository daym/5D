#include "Evaluators/Operation"
#include "Evaluators/Evaluators"
#include "AST/Symbols"
#include "REPL/ExtREPL"

namespace REPLX {
DECLARE_SIMPLE_OPERATION(RImporter)
DECLARE_SIMPLE_OPERATION(RInformant)
DECLARE_SIMPLE_OPERATION(RDefiner)
DECLARE_SIMPLE_OPERATION(ROperatorPrecedenceListGetter)
DECLARE_SIMPLE_OPERATION(RPurger)
DECLARE_SIMPLE_OPERATION(RExecutor)
};
namespace REPL {

static AST::Node* getMethod(AST::Node* name) {
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
	else
		return(NULL); // TODO
}
DEFINE_SIMPLE_OPERATION(REPLMethodGetter, getMethod(Evaluators::reduce(argument)))
REGISTER_BUILTIN(REPLMethodGetter, 1, 0, AST::symbolFromStr("REPLMethods"))

};
