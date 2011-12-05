#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {

static AST::Node* wrapMessageBox(AST::Node* options, AST::Node* argument) {
        HWND cParentWindow = NULL;
        char* cText;
        char* cCaption;
        int cType = 0;
        std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
        AST::Node* parent = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("parent:"));
        if(parent && dynamic_cast<AST::Box*>(parent) != NULL) {
                cParentWindow = (HWND) dynamic_cast<AST::Box*>(parent)->native;
        }
        AST::Node* type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
        cType |= (type_ == AST::symbolFromStr("ok")) ? MB_OK :
                 (type_ == AST::symbolFromStr("okcancel")) ? MB_OKCANCEL :
                         (type_ == AST::symbolFromStr("abortretryignore")) ? MB_ABORTRETRYIGNORE :
                         (type_ == AST::symbolFromStr("yesnocancel")) ? MB_YESNOCANCEL :
                         (type_ == AST::symbolFromStr("yesno")) ? MB_YESNO :
                         (type_ == AST::symbolFromStr("retrycancel")) ? MB_RETRYCANCEL :
                         (type_ == AST::symbolFromStr("canceltrycontinue")) ? MB_CANCELTRYCONTINUE : 0;
        AST::Node* modality = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("modality:"));
        cType |= (modality == AST::symbolFromStr("appl")) ? MB_APPLMODAL :
                (modality == AST::symbolFromStr("system")) ? MB_SYSTEMMODAL :
                (modality == AST::symbolFromStr("task")) ? MB_TASKMODAL :
                0;
        AST::Node* icon = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
        cType |= (icon == AST::symbolFromStr("information")) ? MB_ICONINFORMATION :
                (icon == AST::symbolFromStr("exclamation")) ? MB_ICONEXCLAMATION :
                (icon == AST::symbolFromStr("hand")) ? MB_ICONHAND :
                (icon == AST::symbolFromStr("stop")) ? MB_ICONSTOP :
                (icon == AST::symbolFromStr("question")) ? MB_ICONQUESTION :
                (icon == AST::symbolFromStr("asterisk")) ? MB_ICONASTERISK :
                (icon == AST::symbolFromStr("warning")) ? MB_ICONWARNING :
                (icon == AST::symbolFromStr("error")) ? MB_ICONERROR :
                0; // TODO more
        std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
        cText = Evaluators::get_native_string(iter->second);
        ++iter;
        AST::Node* world = iter->second;
        AST::Node* caption = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("caption:"));
        cCaption = Evaluators::get_native_string(caption);
        fprintf(stdout, "%s [TODO buttons] ", cText);
        fflush(stdout);
        AST::Node* result = Numbers::internNative(gtk_message_dialog(cParentWindow, cText.c_str(), cCaption.c_str(), cType));

        return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
        return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, AST::symbolFromStr("messageBox"))

}; // end namespace
