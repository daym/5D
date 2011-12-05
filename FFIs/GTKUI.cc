#include <gtk/gtk.h>
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {

static AST::Node* wrapMessageBox(AST::Node* options, AST::Node* argument) {
        GtkWindow* cParentWindow = NULL;
        char* cText;
        char* cCaption;
	GtkButtonsType cButtons = GTK_BUTTONS_CLOSE;
        GtkMessageType cType = GTK_MESSAGE_INFO;
        std::list<std::pair<AST::Keyword*, AST::Node*> > arguments = Evaluators::CXXfromArguments(options, argument);
        AST::Node* parent = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("parent:"));
        if(parent && dynamic_cast<AST::Box*>(parent) != NULL) {
                cParentWindow = (GtkWindow*) dynamic_cast<AST::Box*>(parent)->native;
        }
        AST::Node* type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("type:"));
        cButtons = (type_ == AST::symbolFromStr("ok")) ? GTK_BUTTONS_OK:
                 (type_ == AST::symbolFromStr("okcancel")) ? GTK_BUTTONS_OK_CANCEL :
                         (type_ == AST::symbolFromStr("abortretryignore")) ? GTK_BUTTONS_OK_CANCEL :
                         (type_ == AST::symbolFromStr("yesnocancel")) ? GTK_BUTTONS_YES_NO :
                         (type_ == AST::symbolFromStr("yesno")) ? GTK_BUTTONS_YES_NO :
                         (type_ == AST::symbolFromStr("retrycancel")) ? GTK_BUTTONS_CANCEL :
                         (type_ == AST::symbolFromStr("canceltrycontinue")) ? GTK_BUTTONS_CANCEL :
                         GTK_BUTTONS_CLOSE;
        //AST::Node* modality = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("modality:"));
        AST::Node* icon = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
        cType = (icon == AST::symbolFromStr("information")) ? GTK_MESSAGE_INFO :
                (icon == AST::symbolFromStr("exclamation")) ? GTK_MESSAGE_WARNING :
                (icon == AST::symbolFromStr("hand")) ? GTK_MESSAGE_ERROR :
                (icon == AST::symbolFromStr("stop")) ? GTK_MESSAGE_ERROR :
                (icon == AST::symbolFromStr("question")) ? GTK_MESSAGE_QUESTION :
                (icon == AST::symbolFromStr("asterisk")) ? GTK_MESSAGE_ERROR :
                (icon == AST::symbolFromStr("warning")) ? GTK_MESSAGE_WARNING :
                (icon == AST::symbolFromStr("error")) ? GTK_MESSAGE_ERROR :
                GTK_MESSAGE_INFO; // TODO more
        std::list<std::pair<AST::Keyword*, AST::Node*> >::const_iterator iter = arguments.begin();
        cText = Evaluators::get_native_string(iter->second);
        ++iter;
        AST::Node* world = iter->second;
        AST::Node* caption = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("caption:"));
        cCaption = Evaluators::get_native_string(caption);
	GtkWidget* dialog = gtk_message_dialog_new(cParentWindow, (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), cType, cButtons, "%s", cText);
	int result = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
        return(Evaluators::makeIOMonad(Numbers::internNative((Numbers::NativeInt) result), world));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
        return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, AST::symbolFromStr("messageBox"))

}; // end namespace
