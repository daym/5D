#include <string>
#include "stdafx.h"
#include "resource.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "WIN32REPL"
#include "Scanners/MathParser"
#include "AST/AST"

namespace GUI {

static char* ToUTF8(const std::wstring& source) {
	int count = WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, NULL, 0, NULL, NULL);
	char* result = (char*) calloc(count, sizeof(char));
	if(WideCharToMultiByte(CP_UTF8, 0, source.c_str(), -1, result, count, NULL, NULL) != count)
		abort();
	return(result);
}

static std::wstring FromUTF8(const char* source) {
	int count = MultiByteToWideChar(CP_UTF8, 0, source, -1, NULL, 0);
	WCHAR* result = (WCHAR*) calloc(count, sizeof(WCHAR));
	if(MultiByteToWideChar(CP_UTF8, 0, source, -1, result, count) != count)
		abort();
	return(result);
}

std::wstring GetRichTextSelectedText(HWND control) {
	WPARAM beginning = 0;
	LPARAM end = 0;
	WCHAR buffer[20000];
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	if(end <= beginning)
		return(_T(""));
	if(GetWindowText(control, buffer, 20000 - 1) < 1) // FIXME error handling
		return(_T(""));
	return(buffer);
}
void InsertRichText(HWND control, std::wstring value) {
	int iTotalTextLength;
	iTotalTextLength = GetWindowTextLength(control);
	WPARAM beginning = 0;
	LPARAM end = 0;
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	SendMessage(control, EM_SETSEL, (WPARAM) end, (LPARAM) end);
	SendMessage(control, EM_REPLACESEL, (WPARAM)(BOOL)FALSE, (LPARAM)(LPWSTR) value.c_str());
}

std::wstring GetDlgItemTextCXX(HWND dialog, int control) {
	WCHAR buffer[20000] = {0};
	GetDlgItemTextW(dialog, control, buffer, 20000 - 1);
	// TODO error handling (if at all possible).
	return(buffer);
}
void ClearRichTextSelection(HWND control) {
	int iTotalTextLength;
	iTotalTextLength = GetWindowTextLength(control);
	SendMessage(control, EM_SETSEL, (WPARAM)(int)iTotalTextLength, (LPARAM)(int)iTotalTextLength);
}

/** ensures that an entry exists in the environment. */
void EnsureInEnvironment(HWND dialog, const std::wstring& name, AST::Node* value) {
	int index = SendDlgItemMessageW(dialog, IDC_ENVIRONMENT, LB_ADDSTRING, 0, (LPARAM) name.c_str());
	SendDlgItemMessageW(dialog, IDC_ENVIRONMENT, LB_SETITEMDATA, (WPARAM)index, (LPARAM)value);
}
/*
   int iTotalTextLength = GetWindowTextLength(hwnd);
   
 SendMessage(hwnd, EM_SETSEL, (WPARAM)(int)iTotalTextLength, (LPARAM)(int)iTotalTextLength);
   SendMessage(hwnd, EM_REPLACESEL, (WPARAM)(BOOL)FALSE, (LPARAM)(LPCSTR)Text);
   \r\n

   CHARFORMAT cf;
  cf.cbSize      = sizeof(CHARFORMAT);
   cf.dwMask      = CFM_COLOR | CFM_UNDERLINE | CFM_BOLD;
   cf.dwEffects   = (unsigned long)~(CFE_AUTOCOLOR | CFE_UNDERLINE | CFE_BOLD);
   cf.crTextColor = crNewColor;
SendMessage(hwnd, EM_SETSEL, (WPARAM)(int)iStartPos, (LPARAM)(int)iEndPos);
   SendMessage(hwnd, EM_SETCHARFORMAT, (WPARAM)(UINT)SCF_SELECTION, (LPARAM)&cf);
   SendMessage(hwnd, EM_HIDESELECTION, (WPARAM)(BOOL)TRUE, (LPARAM)(BOOL)FALSE);

   SendMessage(hwnd, EM_LINESCROLL, (WPARAM)(int)0, (LPARAM)(int)1);

 */

struct REPL {
	HWND dialog;
	//HACCEL accelerators;
};

void REPL_save(struct REPL* self) {
	// TODO
}

INT_PTR CALLBACK HandleREPLMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	struct REPL* self;
	self = (struct REPL*) GetWindowLongPtr(dialog, GWLP_USERDATA);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE; /* set default input focus */

	case WM_CLOSE:
		EndDialog(dialog, IDCLOSE); /* or rather HideWindows */
		return (INT_PTR)TRUE;

	case WM_SIZE:
		{
			RECT clientRect;
			RECT executeButtonRect;
			RECT saveButtonRect;
			RECT openButtonRect;
			RECT windowRect;
			RECT outputRect;
			RECT commandEntryRect;
			RECT environmentRect;
			GetClientRect(dialog, &clientRect);
			int cx = clientRect.right - clientRect.left;
			int cy = clientRect.bottom - clientRect.top;
			GetWindowRect(GetDlgItem(dialog, IDC_EXECUTE), &executeButtonRect);
			GetWindowRect(GetDlgItem(dialog, IDC_SAVE), &saveButtonRect);
			GetWindowRect(GetDlgItem(dialog, IDC_OPEN), &openButtonRect);
			GetWindowRect(GetDlgItem(dialog, IDC_OUTPUT), &outputRect);
			GetWindowRect(GetDlgItem(dialog, IDC_COMMAND_ENTRY), &commandEntryRect);
			GetWindowRect(GetDlgItem(dialog, IDC_ENVIRONMENT), &environmentRect);
			cx -= outputRect.left;
			cy -= outputRect.top;
			SetWindowPos(GetDlgItem(dialog, IDC_SAVE), NULL, saveButtonRect.left, clientRect.bottom - (saveButtonRect.bottom - saveButtonRect.top), 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(dialog, IDC_OUTPUT), NULL, 0, 0, cx, cy, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			//GetDlgItem(self->dialog, IDC_OUTPUT)
		}
		return (INT_PTR)TRUE;

	case WM_NEXTDLGCTL:
		{/* WPARAM */
			ClearRichTextSelection(GetDlgItem(self->dialog, IDC_OUTPUT));
		}
		return (INT_PTR)FALSE;

	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDC_SAVE:
			{
				REPL_save(self);
				break;
			}
		case IDC_EXECUTE:
			{
				std::wstring text;
				HWND output;
				output = GetDlgItem(self->dialog, IDC_OUTPUT);
				text = GetRichTextSelectedText(output);
				if(text.length() == 0)
					text = GetDlgItemTextCXX(self->dialog, IDC_COMMAND_ENTRY);
				std::string UTF8_text = ToUTF8(text);
				REPL_execute(self, UTF8_text.c_str());
				break;
			}
		}

		/*if (LOWORD(wParam) == IDCLOSE)
		{
			EndDialog(dialog, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}*/
		break;
	}
	//return(DefDlgProc(dialog, message, wParam, lParam));
	return (INT_PTR)FALSE;
}

struct REPL* REPL_new(HWND parent) {
	struct REPL* result;
	result = (struct REPL*) calloc(1, sizeof(struct REPL));
	REPL_init(result, parent);
	return(result);
}

void REPL_init(struct REPL* self, HWND parent) {
	HINSTANCE hinstance;
	hinstance = GetModuleHandle(NULL);
	//DialogBox(hinstance, MAKEINTRESOURCE(IDD_REPL), parent, HandleREPLMessage);
	self->dialog = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_REPL), parent, HandleREPLMessage);
	//self->accelerators = LoadAccelerators(hinstance, MAKEINTRESOURCE(IDC_MY4D));
	SetWindowLongPtr(self->dialog, GWLP_USERDATA, (LONG) self);
	ShowWindow(self->dialog, SW_SHOWNORMAL);
}

void REPL_add_to_environment(struct REPL* self, AST::Node* definition) {
	AST::Cons* definitionCons = dynamic_cast<AST::Cons*>(definition);
	if(!definitionCons || definitionCons->head != AST::intern("define") || !definitionCons->tail)
		return;
	AST::Symbol* parameter = dynamic_cast<AST::Symbol*>(definitionCons->tail->head);
	if(!parameter)
		return;
	AST::Node* body = definitionCons->tail->tail;
	if(!body)
		return;
	//std::string bodyString = body->str();
	EnsureInEnvironment(self->dialog, FromUTF8(parameter->name), body);
}

void REPL_set_file_modified(struct REPL* self, bool value) {
	// TODO
}
void REPL_insert_output(struct REPL* self, const char* output) {
	InsertRichText(GetDlgItem(self->dialog, IDC_OUTPUT), FromUTF8(output));
}
void REPL_execute(struct REPL* self, const char* command) {
	Scanners::MathParser parser;
	FILE* input_file = fmemopen((void*) command, strlen(command), "r");
	if(input_file) {
		try {
			try {
				AST::Node* result = parser.parse(input_file);
				REPL_add_to_environment(self, result);
				std::string v = result ? result->str() : "OK";
				v = " => " + v + "\n";
				REPL_insert_output(self, v.c_str());
			} catch(...) {
				fclose(input_file);
				throw;
			}
		} catch(Scanners::ParseException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			REPL_insert_output(self, v.c_str());
		}
		REPL_set_file_modified(self, true);
	}
}

}; // end namespace GUI
