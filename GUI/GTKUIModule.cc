#include <gtk/gtk.h>
#include "Values/Values"
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {
using namespace Values;

static NodeT wrapMessageBox(NodeT options, NodeT argument) {
	GtkWindow* cParentWindow = NULL;
	char* cText;
	char* cCaption;
	GtkButtonsType cButtons = GTK_BUTTONS_CLOSE;
	GtkMessageType cType = GTK_MESSAGE_INFO;
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	NodeT parent = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("parent:"));
	if(!nil_P(parent)) {
		cParentWindow = (GtkWindow*) Evaluators::get_pointer(parent);
	}
	NodeT type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("type:"));
	cButtons = (type_ == symbolFromStr("ok")) ? GTK_BUTTONS_OK:
		 (type_ == symbolFromStr("okcancel")) ? GTK_BUTTONS_OK_CANCEL :
	         (type_ == symbolFromStr("abortretryignore")) ? GTK_BUTTONS_OK_CANCEL :
	         (type_ == symbolFromStr("yesnocancel")) ? GTK_BUTTONS_YES_NO :
	         (type_ == symbolFromStr("yesno")) ? GTK_BUTTONS_YES_NO :
	         (type_ == symbolFromStr("retrycancel")) ? GTK_BUTTONS_CANCEL :
	         (type_ == symbolFromStr("canceltrycontinue")) ? GTK_BUTTONS_CANCEL :
	         GTK_BUTTONS_CLOSE;
	//NodeT modality = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("modality:"));
	NodeT icon = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("icon:"));
	cType = (icon == symbolFromStr("information")) ? GTK_MESSAGE_INFO :
	        (icon == symbolFromStr("exclamation")) ? GTK_MESSAGE_WARNING :
	        (icon == symbolFromStr("hand")) ? GTK_MESSAGE_ERROR :
	        (icon == symbolFromStr("stop")) ? GTK_MESSAGE_ERROR :
	        (icon == symbolFromStr("question")) ? GTK_MESSAGE_QUESTION :
	        (icon == symbolFromStr("asterisk")) ? GTK_MESSAGE_ERROR :
	        (icon == symbolFromStr("warning")) ? GTK_MESSAGE_WARNING :
	        (icon == symbolFromStr("error")) ? GTK_MESSAGE_ERROR :
	        GTK_MESSAGE_INFO; // TODO more
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	cText = Evaluators::get_string(iter->second);
	FETCH_WORLD(iter);
	NodeT caption = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("caption:"));
	cCaption = caption ? Evaluators::get_string(caption) : NULL;
	GtkWidget* dialog = gtk_message_dialog_new(cParentWindow, (GtkDialogFlags) (GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), cType, cButtons, "%s", cText);
	if(cCaption)
		gtk_window_set_title(GTK_WINDOW(dialog), cCaption);
	int cResult = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(GTK_WIDGET(dialog));
	NodeT result;
	result = (cResult == GTK_RESPONSE_NONE) ? symbolFromStr("none") :
	         (cResult == GTK_RESPONSE_REJECT) ? symbolFromStr("reject") : 
	         (cResult == GTK_RESPONSE_ACCEPT) ? symbolFromStr("accept") : 
	         (cResult == GTK_RESPONSE_DELETE_EVENT) ? symbolFromStr("deleted") : 
	         (cResult == GTK_RESPONSE_OK) ? symbolFromStr("ok") : 
	         (cResult == GTK_RESPONSE_CANCEL) ? symbolFromStr("cancel") : 
	         (cResult == GTK_RESPONSE_CLOSE) ? symbolFromStr("close") : 
	         (cResult == GTK_RESPONSE_YES) ? symbolFromStr("yes") : 
	         (cResult == GTK_RESPONSE_NO) ? symbolFromStr("no") : 
	         (cResult == GTK_RESPONSE_APPLY) ? symbolFromStr("apply") : 
	         (cResult == GTK_RESPONSE_HELP) ? symbolFromStr("help") : 
	         symbolFromStr("unknown");
	return(CHANGED_WORLD(result));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
	return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, symbolFromStr("messageBox!"))

}; // end namespace
