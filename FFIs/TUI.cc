#include <stdio.h>
#include <string.h>
#include "AST/AST"
#include "AST/Symbol"
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {

static AST::Node* wrapMessageBox(AST::Node* options, AST::Node* argument) {
        char* cText;
        char* cCaption;
        const char* buttons;
        std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
        AST::Node* parent = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("parent:"));
        AST::Node* type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("type:"));
        AST::Node* modality = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("modality:"));
        AST::Node* icon = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
        const char* cIcon;
        buttons = (type_ == AST::symbolFromStr("ok")) ? "ok" :
                 (type_ == AST::symbolFromStr("okcancel")) ? "ok/cancel" :
                         (type_ == AST::symbolFromStr("abortretryignore")) ? "abort/retry/ignore" :
                         (type_ == AST::symbolFromStr("yesnocancel")) ? "yes/no/cancel" :
                         (type_ == AST::symbolFromStr("yesno")) ? "yes/no" :
                         (type_ == AST::symbolFromStr("retrycancel")) ? "retry/cancel" :
                         (type_ == AST::symbolFromStr("canceltrycontinue")) ? "cancel|try|continue" : 
                    "ok";
        cIcon = (icon == AST::symbolFromStr("information")) ? "(i) " :
                (icon == AST::symbolFromStr("exclamation")) ? "(!) " :
                (icon == AST::symbolFromStr("hand")) ? "(H) " :
                (icon == AST::symbolFromStr("stop")) ? "(S) " :
                (icon == AST::symbolFromStr("question")) ? "(?) " :
                (icon == AST::symbolFromStr("asterisk")) ? "(*) " :
                (icon == AST::symbolFromStr("warning")) ? "(W)" :
                (icon == AST::symbolFromStr("error")) ? "(E) " :
                ""; // TODO more
        std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
        cText = Evaluators::get_native_string(iter->second);
        ++iter;
        AST::Node* world = iter->second;
        AST::Node* caption = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("caption:"));
        cCaption = Evaluators::get_native_string(caption);
        char buffer[21];
        AST::Symbol* result = AST::symbolFromStr("close");
        while(true) {
                fprintf(stdout, "%s%s (%s) ", cIcon, cText, buttons);
                fflush(stdout);
                if(!fgets(buffer, 20, stdin)) {
                        result = AST::symbolFromStr("close");
                        break;
                }
                if(buffer[0] == 10) {
                        buffer[0] = buttons[0];
                        buffer[1] = 0;
                }
                switch(buffer[0]) {
                case 'o':
                        result = AST::symbolFromStr("ok");
                        break;
                case 'c':
                        result = AST::symbolFromStr("cancel");
                        break;
                case 'y':
                        result = AST::symbolFromStr("yes");
                        break;
                case 'n':
                        result = AST::symbolFromStr("no");
                        break;
                case 'a':
                        result = AST::symbolFromStr("abort");
                        break;
                case 'r':
                        result = AST::symbolFromStr("retry");
                        break;
                case 'i':
                        result = AST::symbolFromStr("ignore");
                        break;
                case 't':
                        result = AST::symbolFromStr("try");
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
        return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
        return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, AST::symbolFromStr("messageBox"))

}; // end namespace
