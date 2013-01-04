#include "stdafx.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include "FFIs/UI"
#include <5D/Operations>
#include <5D/FFIs>
#include "Evaluators/Builtins"
#include "Values/Values"
namespace FFIs {
using namespace Values;
#ifndef MB_CANCELTRYCONTINUE
#define MB_CANCELTRYCONTINUE 0x00000006L
#endif
#ifndef IDTRYAGAIN
#define IDTRYAGAIN 10
#endif
#ifndef IDCONTINUE
#define IDCONTINUE   11
#endif

static NodeT wrapMessageBox(NodeT options, NodeT argument) {
	HWND cParentWindow = NULL;
	std::wstring cText;
	std::wstring cCaption;
	int cType = 0;
	Evaluators::CXXArguments arguments = Evaluators::CXXfromArguments(options, argument);
	NodeT parent = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("parent:"));
	if(parent) {
		cParentWindow = (HWND) Values::pointerFromNode(parent);
	}
	NodeT type_ = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("type:"));
	cType |= (type_ == symbolFromStr("ok")) ? MB_OK :
	         (type_ == symbolFromStr("okcancel")) ? MB_OKCANCEL :
			 (type_ == symbolFromStr("abortretryignore")) ? MB_ABORTRETRYIGNORE :
			 (type_ == symbolFromStr("yesnocancel")) ? MB_YESNOCANCEL :
			 (type_ == symbolFromStr("yesno")) ? MB_YESNO :
			 (type_ == symbolFromStr("retrycancel")) ? MB_RETRYCANCEL :
			 (type_ == symbolFromStr("canceltrycontinue")) ? MB_CANCELTRYCONTINUE : 0;
	NodeT modality = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("modality:"));
	cType |= (modality == symbolFromStr("appl")) ? MB_APPLMODAL :
		(modality == symbolFromStr("system")) ? MB_SYSTEMMODAL : 
		(modality == symbolFromStr("task")) ? MB_TASKMODAL : 
		0;
	NodeT icon = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("icon:"));
	cType |= (icon == symbolFromStr("information")) ? MB_ICONINFORMATION :
		(icon == symbolFromStr("exclamation")) ? MB_ICONEXCLAMATION : 
		(icon == symbolFromStr("hand")) ? MB_ICONHAND : 
		(icon == symbolFromStr("stop")) ? MB_ICONSTOP : 
		(icon == symbolFromStr("question")) ? MB_ICONQUESTION : 
		(icon == symbolFromStr("asterisk")) ? MB_ICONASTERISK : 
		(icon == symbolFromStr("warning")) ? MB_ICONWARNING : 
		(icon == symbolFromStr("error")) ? MB_ICONERROR : 
		0; // TODO more
	Evaluators::CXXArguments::const_iterator iter = arguments.begin();
	cText = wstringFromUtf8(Values::stringFromNode(iter->second));
	FETCH_WORLD(iter);
	NodeT caption = Evaluators::CXXgetKeywordArgumentValue(arguments, keywordFromStr("caption:"));
	cCaption = caption ? wstringFromUtf8(Values::stringFromNode(caption)) : std::wstring();
	int cResult = MessageBoxW(cParentWindow, cText.c_str(), cCaption.c_str(), cType);
	NodeT result;
	result = (cResult == IDOK) ? symbolFromStr("ok") : 
	         (cResult == IDCANCEL) ? symbolFromStr("cancel") : 
	         (cResult == IDYES) ? symbolFromStr("yes") : 
	         (cResult == IDNO) ? symbolFromStr("no") : 
	         (cResult == IDABORT) ? symbolFromStr("abort") : 
	         (cResult == IDRETRY) ? symbolFromStr("retry") : 
	         (cResult == IDIGNORE) ? symbolFromStr("ignore") : 
	         (cResult == IDTRYAGAIN) ? symbolFromStr("try") : 
	         (cResult == IDCONTINUE) ? symbolFromStr("continue") : 
	         symbolFromStr("unknown");

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
	return(CHANGED_WORLD(result));
}
DEFINE_FULL_OPERATION(MessageBoxDisplayer, {
	return(wrapMessageBox(fn, argument));
})
REGISTER_BUILTIN(MessageBoxDisplayer, (-2), 0, symbolFromStr("messageBox!"))

}; // end namespace
