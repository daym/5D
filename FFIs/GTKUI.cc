#include <gtk/gtk.h>
#include "Values/Values"
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {

static AST::NodeT wrapMessageBox(AST::NodeT options, AST::NodeT argument) {
	GtkWindow* cParentWindow = NULL;
	char* cText;
	char* cCaption;
	GtkButtonsType cButtons = GTK_BUTTONS_CLOSE;
	GtkMessageType cType = GTK_MESSAGE_INFO;
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	AST::NodeT parent = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("parent:"));
	if(!nil_P(parent)) {
		cParentWindow = (GtkWindow*) Evaluators::get_pointer(parent);
	}
	AST::NodeT type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("type:"));
	cButtons = (type_ == AST::symbolFromStr("ok")) ? GTK_BUTTONS_OK:
		 (type_ == AST::symbolFromStr("okcancel")) ? GTK_BUTTONS_OK_CANCEL :
	         (type_ == AST::symbolFromStr("abortretryignore")) ? GTK_BUTTONS_OK_CANCEL :
	         (type_ == AST::symbolFromStr("yesnocancel")) ? GTK_BUTTONS_YES_NO :
	         (type_ == AST::symbolFromStr("yesno")) ? GTK_BUTTONS_YES_NO :
	         (type_ == AST::symbolFromStr("retrycancel")) ? GTK_BUTTONS_CANCEL :
	         (type_ == AST::symbolFromStr("canceltrycontinue")) ? GTK_BUTTONS_CANCEL :
	         GTK_BUTTONS_CLOSE;
	//AST::NodeT modality = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("modality:"));
	AST::NodeT icon = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
	cType = (icon == AST::symbolFromStr("information")) ? GTK_MESSAGE_INFO :
	        (icon == AST::symbolFromStr("exclamation")) ? GTK_MESSAGE_WARNING :
	        (icon == AST::symbolFromStr("hand")) ? GTK_MESSAGE_ERROR :
	        (icon == AST::symbolFromStr("stop")) ? GTK_MESSAGE_ERROR :
	        (icon == AST::symbolFromStr("question")) ? GTK_MESSAGE_QUESTION :
	        (icon == AST::symbolFromStr("asterisk")) ? GTK_MESSAGE_ERROR :
	        (icon == AST::symbolFromStr("warning")) ? GTK_MESSAGE_WARNING :
	        (icon == AST::symbolFromStr("error")) ? GTK_MESSAGE_ERROR :
	        GTK_MESSAGE_INFO; // TODO more
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	cText = Evaluators::get_string(iter->second);
	FETCH_WORLD(iter);
	AST::NodeT caption = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("caption:"));
	cCaption = caption ? Evaluators::get_string(caption) : NULL;
	GtkWidget* dialog = gtk_message_dialog_new(cParentWindow, (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), cType, cButtons, "%s", cText);
	if(cCaption)
		gtk_window_set_title(GTK_WINDOW(dialog), cCaption);
	int cResult = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	AST::NodeT result;
	result = (cResult == GTK_RESPONSE_NONE) ? AST::symbolFromStr("none") :
	         (cResult == GTK_RESPONSE_REJECT) ? AST::symbolFromStr("reject") : 
	         (cResult == GTK_RESPONSE_ACCEPT) ? AST::symbolFromStr("accept") : 
	         (cResult == GTK_RESPONSE_DELETE_EVENT) ? AST::symbolFromStr("deleted") : 
	         (cResult == GTK_RESPONSE_OK) ? AST::symbolFromStr("ok") : 
	         (cResult == GTK_RESPONSE_CANCEL) ? AST::symbolFromStr("cancel") : 
	         (cResult == GTK_RESPONSE_CLOSE) ? AST::symbolFromStr("close") : 
	         (cResult == GTK_RESPONSE_YES) ? AST::symbolFromStr("yes") : 
	         (cResult == GTK_RESPONSE_NO) ? AST::symbolFromStr("no") : 
	         (cResult == GTK_RESPONSE_APPLY) ? AST::symbolFromStr("apply") : 
	         (cResult == GTK_RESPONSE_HELP) ? AST::symbolFromStr("help") : 
	         AST::symbolFromStr("unknown");
	return(CHANGED_WORLD(result));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
	return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, AST::symbolFromStr("messageBox!"))

}; // end namespace
