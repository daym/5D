#include <stdio.h>
#include <string.h>
#include "Values/Values"
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {
using namespace Values;

static NodeT wrapMessageBox(NodeT options, NodeT argument) {
	char* cText;
	char* cCaption;
	const char* buttons;
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	//NodeT parent = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("parent:"));
	NodeT type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("type:"));
	//NodeT modality = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("modality:"));
	NodeT icon = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("icon:"));
	const char* cIcon;
	buttons = (type_ == symbolFromStr("ok")) ? "ok" :
		 (type_ == symbolFromStr("okcancel")) ? "ok/cancel" :
			 (type_ == symbolFromStr("abortretryignore")) ? "abort/retry/ignore" :
			 (type_ == symbolFromStr("yesnocancel")) ? "yes/no/cancel" :
			 (type_ == symbolFromStr("yesno")) ? "yes/no" :
			 (type_ == symbolFromStr("retrycancel")) ? "retry/cancel" :
			 (type_ == symbolFromStr("canceltrycontinue")) ? "cancel/try/continue" : 
		    "close";
	cIcon = (icon == symbolFromStr("information")) ? "(i) " :
		(icon == symbolFromStr("exclamation")) ? "(!) " :
		(icon == symbolFromStr("hand")) ? "(H) " :
		(icon == symbolFromStr("stop")) ? "(S) " :
		(icon == symbolFromStr("question")) ? "(?) " :
		(icon == symbolFromStr("asterisk")) ? "(*) " :
		(icon == symbolFromStr("warning")) ? "(W)" :
		(icon == symbolFromStr("error")) ? "(E) " :
		""; // TODO more
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	cText = Evaluators::get_string(iter->second);
	FETCH_WORLD(iter);
	NodeT caption = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("caption:"));
	cCaption = caption ? Evaluators::get_string(caption) : NULL;
	char buffer[21];
	NodeT result = symbolFromStr("close");
	while(true) {
		if(cCaption)
			fprintf(stdout, "= %s =\n", cCaption);
		fprintf(stdout, "%s%s (%s) ", cIcon, cText, buttons);
		fflush(stdout);
		if(!fgets(buffer, 20, stdin)) {
			result = symbolFromStr("close");
			break;
		}
		if(buffer[0] == 10) {
			buffer[0] = buttons[0] == 'c' && buttons[1] == 'l' ? 'C' : buttons[0];
			buffer[1] = 0;
		}
		switch(buffer[0]) {
		case 'o':
			result = symbolFromStr("ok");
			break;
		case 'c':
			result = (buffer[1] == 'l') ? symbolFromStr("close") : symbolFromStr("cancel");
			break;
		case 'C':
			result = symbolFromStr("close");
			break;
		case 'y':
			result = symbolFromStr("yes");
			break;
		case 'n':
			result = symbolFromStr("no");
			break;
		case 'a':
			result = symbolFromStr("abort");
			break;
		case 'r':
			result = symbolFromStr("retry");
			break;
		case 'i':
			result = symbolFromStr("ignore");
			break;
		case 't':
			result = symbolFromStr("try");
			break;
		default:
			result = NULL;
		}
		if(result == NULL) {
			fprintf(stdout, "What?\n");
			fflush(stdout);
		} else
			break;
	}
	return(CHANGED_WORLD(result));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
	return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, symbolFromStr("messageBox!"))

}; // end namespace
