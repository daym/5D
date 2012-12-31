#include <5D/Operations>
#include "REPL/Symbols"

namespace REPLX {
using namespace Values;
BEGIN_PROC_WRAPPER(parse, 1, symbolFromStr("parse!"), static)
	makeCall makeMathParser Sparse
END_PROC_WRAPPER
/*static BEGIN_PROC_WRAPPER(inform)
END_PROC_WRAPPER*/
BEGIN_PROC_WRAPPER(describe, 1, symbolFromStr("describe!"), static)
END_PROC_WRAPPER
BEGIN_PROC_WRAPPER(define, 2, symbolFromStr("define!"), static)
END_PROC_WRAPPER
BEGIN_PROC_WRAPPER(getOperatorPrecedenceList, 0, symbolFromStr("getOperatorPrecedenceList!"))
END_PROC_WRAPPER
BEGIN_PROC_WRAPPER(purge, 0, symbolFromStr("purge!"), static)
END_PROC_WRAPPER
BEGIN_PROC_WRAPPER(execute, 1, symbolFromStr("execute!"), static)
END_PROC_WRAPPER
BEGIN_PROC_WRAPPER(import, 1, symbolFromStr("import!"), static)
END_PROC_WRAPPER
Values::NodeT dispatchREPL = Evaluators::eval(Values::makeApplication(dispatch, exportsf("%s!", &push, &pop, &getPosition, &getColumnNumber, &getLineNumber, &raiseError, &EOFP, &ensureEnd, &consume, &setHonorIndentation, &getInputValue, &parseMatchingParens)), NULL);
BEGIN_PROC_WRAPPER(makeREPL, 0, symbolFromStr("makeREPL!"), )
END_PROC_WRAPPER

};
