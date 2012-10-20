#include "Evaluators/Operation"
#include "Evaluators/Evaluators"
#include "Values/Symbols"
#include "REPL/ExtREPL"

namespace REPLX {
DECLARE_SIMPLE_OPERATION(RImporter)
DECLARE_SIMPLE_OPERATION(RInformant)
DECLARE_SIMPLE_OPERATION(RDefiner)
DECLARE_SIMPLE_OPERATION(ROperatorPrecedenceListGetter)
DECLARE_SIMPLE_OPERATION(RPurger)
DECLARE_SIMPLE_OPERATION(RExecutor)

static AST::NodeT getMethod(AST::NodeT name) {
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
		return(AST::makeCons(Symbols::SgetOperatorPrecedenceListexclam,
		       AST::makeCons(Symbols::Sdescribeexclam,
		       AST::makeCons(Symbols::Sdefineexcam,
		       AST::makeCons(Symbols::Simportexclam,
		       AST::makeCons(Symbols::Spurgeexclam,
		       AST::makeCons(Symbols::Sexecuteexclam,
		       AST::makeCons(Symbols::Sexports,
		       NULL))))))));
	else
		return(NULL); // TODO
}
DEFINE_SIMPLE_OPERATION(REPLMethodGetter, getMethod(Evaluators::reduce(argument)))
REGISTER_BUILTIN(REPLMethodGetter, 1, 0, AST::symbolFromStr("REPLMethods"))

};
