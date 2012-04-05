#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "FFIs/UI"
#include "Evaluators/Operation"
#include "Evaluators/FFI"
#include "Evaluators/Builtins"
namespace FFIs {

static AST::NodeT wrapMessageBox(AST::NodeT options, AST::NodeT argument) {
	HWND cParentWindow = NULL;
	std::wstring cText;
	std::wstring cCaption;
	int cType = 0;
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	AST::NodeT parent = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("parent:"));
	if(parent) {
		cParentWindow = (HWND) Evaluators::get_pointer(parent);
	}
	AST::NodeT type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("type:"));
	cType |= (type_ == AST::symbolFromStr("ok")) ? MB_OK :
	         (type_ == AST::symbolFromStr("okcancel")) ? MB_OKCANCEL :
			 (type_ == AST::symbolFromStr("abortretryignore")) ? MB_ABORTRETRYIGNORE :
			 (type_ == AST::symbolFromStr("yesnocancel")) ? MB_YESNOCANCEL :
			 (type_ == AST::symbolFromStr("yesno")) ? MB_YESNO :
			 (type_ == AST::symbolFromStr("retrycancel")) ? MB_RETRYCANCEL :
			 (type_ == AST::symbolFromStr("canceltrycontinue")) ? MB_CANCELTRYCONTINUE : 0;
	AST::NodeT modality = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("modality:"));
	cType |= (modality == AST::symbolFromStr("appl")) ? MB_APPLMODAL :
		(modality == AST::symbolFromStr("system")) ? MB_SYSTEMMODAL : 
		(modality == AST::symbolFromStr("task")) ? MB_TASKMODAL : 
		0;
	AST::NodeT icon = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("icon:"));
	cType |= (icon == AST::symbolFromStr("information")) ? MB_ICONINFORMATION :
		(icon == AST::symbolFromStr("exclamation")) ? MB_ICONEXCLAMATION : 
		(icon == AST::symbolFromStr("hand")) ? MB_ICONHAND : 
		(icon == AST::symbolFromStr("stop")) ? MB_ICONSTOP : 
		(icon == AST::symbolFromStr("question")) ? MB_ICONQUESTION : 
		(icon == AST::symbolFromStr("asterisk")) ? MB_ICONASTERISK : 
		(icon == AST::symbolFromStr("warning")) ? MB_ICONWARNING : 
		(icon == AST::symbolFromStr("error")) ? MB_ICONERROR : 
		0; // TODO more
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	cText = FromUTF8(Evaluators::get_string(iter->second));
	++iter;
	AST::NodeT world = iter->second;
	AST::NodeT caption = Evaluators::CXXgetKeywordArgumentValue(arguments, AST::keywordFromStr("caption:"));
	cCaption = caption ? FromUTF8(Evaluators::get_string(caption)) : _T("");
	int cResult = MessageBoxW(cParentWindow, cText.c_str(), cCaption.c_str(), cType);
	AST::Symbol* result;
	result = (cResult == IDOK) ? AST::symbolFromStr("ok") : 
	         (cResult == IDCANCEL) ? AST::symbolFromStr("cancel") : 
	         (cResult == IDYES) ? AST::symbolFromStr("yes") : 
	         (cResult == IDNO) ? AST::symbolFromStr("no") : 
	         (cResult == IDABORT) ? AST::symbolFromStr("abort") : 
	         (cResult == IDRETRY) ? AST::symbolFromStr("retry") : 
	         (cResult == IDIGNORE) ? AST::symbolFromStr("ignore") : 
	         (cResult == IDTRYAGAIN) ? AST::symbolFromStr("try") : 
	         (cResult == IDCONTINUE) ? AST::symbolFromStr("continue") : 
	         AST::symbolFromStr("unknown");

	/* TODO 
	#define MB_NOFOCUS                  0x00008000L
#define MB_SETFOREGROUND            0x00010000L
#define MB_DEFAULT_DESKTOP_ONLY     0x00020000L

#if(WINVER >= 0x0400)
#define MB_TOPMOST                  0x00040000L
#define MB_RIGHT                    0x00080000L
#define MB_RTLREADING               0x00100000L

	MB_SERVICE_NOTIFICATION          

#define MB_HELP                     0x00004000L // Help Button
#define MB_DEFBUTTON1               0x00000000L
#define MB_DEFBUTTON2               0x00000100L
#define MB_DEFBUTTON3               0x00000200L
#if(WINVER >= 0x0400)
#define MB_DEFBUTTON4               0x00000300L
#endif

	*/
	return(Evaluators::makeIOMonad(result, world));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
	return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, AST::symbolFromStr("messageBox!"))

}; // end namespace
