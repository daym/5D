#include <string>
#include <stdio.h>
#include <assert.h>
#include "stdafx.h"
#include "resource.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <Commdlg.h>
#include <Richedit.h>
#include "REPL"
#include "WIN32REPL"
#include "Scanners/MathParser"
#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Config/Config"
#include "FFIs/FFIs"
#include "Evaluators/Builtins"
#include "GUI/WIN32Completer"

namespace GUI {
void REPL_append_to_output_buffer(struct REPL* self, const char* text);


static void ShowWIN32Diagnostics(void) {
	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 
	if(FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL) > 0) {
		MessageBoxW(NULL, (LPCWSTR)lpMsgBuf, _T("Error"), MB_OK);
		LocalFree(lpMsgBuf);
	}
	//ExitProcess(dw); 
}
int GetRichTextCaretPosition(HWND control) {
	WPARAM beginning = 0;
	LPARAM end = 0;
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	return(beginning);
}
std::wstring GetRichTextSelectedText(HWND control) {
	WPARAM beginning = 0;
	LPARAM end = 0;
	WCHAR buffer[20000];
	SendMessage(control, EM_GETSEL, (WPARAM) &beginning, (LPARAM) &end);
	if(end <= beginning)
		return(std::wstring());
	if(GetWindowTextW(control, buffer, 20000 - 1) < 1) // FIXME error handling
		return(std::wstring());
	return(buffer);
}
static OPENFILENAMEW openFileName;
static WCHAR openFileNameName[2049];
static WCHAR openFileNameFilter[] = _T("All Files (*.*)\0*.*\0\065D Source Files (*.5D)\0*.5D\0\0");
static void initializeOpenFileNameStruct() {
	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFile = openFileNameName;
	openFileName.lpstrFile[0] = '\0';
	openFileName.nMaxFile = sizeof(openFileNameName) / sizeof(openFileNameName[0]);
	openFileName.lpstrFilter = openFileNameFilter;
	openFileName.nFilterIndex = 2;
	openFileName.lpstrCustomFilter = NULL;
	openFileName.lpstrFileTitle = NULL;
	openFileName.nMaxFileTitle = 0;
	openFileName.lpstrInitialDir = NULL;
	openFileName.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
}
std::wstring GetUsualOpenFileName(HWND hwndOwner) {
	openFileName.hwndOwner = hwndOwner;
	if(!openFileName.lpstrFile)
		initializeOpenFileNameStruct();
	if(GetOpenFileNameW(&openFileName)) {
		return(openFileName.lpstrFile);
	} else
		return(std::wstring());
}
std::wstring GetUsualSaveFileName(HWND hwndOwner) {
	openFileName.hwndOwner = hwndOwner;
	if(!openFileName.lpstrFile)
		initializeOpenFileNameStruct();
	if(GetSaveFileNameW(&openFileName)) {
		return(openFileName.lpstrFile);
	} else
		return(std::wstring());
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
bool GetDlgItemCheckedCXX(HWND dialog, int control) {
	return(SendMessageW(GetDlgItem(dialog, control), BM_GETCHECK, 0, 0) == BST_CHECKED);
}
void SetDlgItemCheckedCXX(HWND dialog, int control, bool value) {
	SendMessageW(GetDlgItem(dialog, control), BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0);
}
void SetDlgItemTextCXX(HWND dialog, int control, const std::wstring& value) {
	SetDlgItemTextW(dialog, control, value.c_str());
	// TODO error handling (if at all possible).
	//return(buffer);
}
static void SetDialogFocus(HWND dialog, int control) {
	SendMessage(dialog, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(dialog, control), (LPARAM) TRUE); 
}
void ClearRichTextSelection(HWND control) {
	int iTotalTextLength;
	iTotalTextLength = GetWindowTextLength(control); // FIXME W
	SendMessage(control, EM_SETSEL, (WPARAM)(int)iTotalTextLength, (LPARAM)(int)iTotalTextLength);
}
static std::wstring GetListBoxEntryStringCXX(HWND control, int index) {
	std::wstring result;
	int length = SendMessageW(control, LB_GETTEXTLEN, (WPARAM) index, (LPARAM) 0);
	TCHAR* buffer = new TCHAR[length + 1];
	buffer[0] = 0;
	SendMessageW(control, LB_GETTEXT, (WPARAM) index, (LPARAM) buffer);
	result = buffer;
	delete buffer;
	return(result);
}
/** ensures that an entry exists in the environment. */
void EnsureInEnvironment(HWND dialog, const std::wstring& name) {
	int index = SendDlgItemMessageW(dialog, IDC_ENVIRONMENT, LB_ADDSTRING, 0, (LPARAM) name.c_str());
	SendDlgItemMessageW(dialog, IDC_ENVIRONMENT, LB_SETITEMDATA, (WPARAM)index, (LPARAM)0);
}
/*    AST::Node* hData = (AST::Node*) SendMessage(hList, LB_GETITEMDATA, (WPARAM)index, 0); */
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
	bool B_file_modified;
	struct Config* fConfig;
	HWND fSearchDialog;
	//HACCEL accelerators;
	std::wstring fSearchTerm;
	bool fBSearchUpwards;
	bool fBSearchCaseSensitive;
	AST::Cons* fTailEnvironment;
	AST::Cons* fTailUserEnvironment /* =fTailBuiltinEnvironmentFrontier */;
	AST::Cons* fTailUserEnvironmentFrontier;
	WNDPROC oldEditBoxProc;
	struct Completer* fCompleter;
	struct std::set<AST::Symbol*>* fEnvironmentKeys;
};

HWND REPL_get_window(struct REPL* self) {
	return(self->dialog);
}
HWND REPL_get_search_window(struct REPL* self) {
	return(self->fSearchDialog);
}
std::wstring REPL_get_absolute_pathw(const std::wstring& file_name) {
	return(file_name);
}
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	std::wstring text = FromUTF8(absolute_name);
	SetWindowText(self->dialog, text.c_str());
	Config_set_environment_name(self->fConfig, absolute_name);
	Config_save(self->fConfig);
}
void REPL_set_file_modified(struct REPL* self, bool value) {
	self->B_file_modified = value;
}
bool REPL_save(struct REPL* self, bool B_force_save_dialog) {
	const char* environmentName;
	std::wstring file_name;
	environmentName = Config_get_environment_name(self->fConfig);
	if(!environmentName || B_force_save_dialog)
		file_name = GetUsualSaveFileName(self->dialog);
	else
		file_name = FromUTF8(environmentName);
	if(file_name.length() > 0) {
		bool B_OK = false;
		// FIXME save into temp file first.
		FILE* output_file = _wfopen(file_name.c_str(), _T("w"));
		if(REPL_save_contents_to(self, output_file)) {
			fclose(output_file);
			char* absolute_name = ToUTF8(REPL_get_absolute_pathw(file_name));
			B_OK = true;
			REPL_set_current_environment_name(self, absolute_name);
			REPL_set_file_modified(self, false);
			//unlink(temp_name);
		}
		return(B_OK);
	} else 
		return(false);
}
bool REPL_load_contents_by_name(struct REPL* self, const char* name) {
	if(!REPL_load_contents_from(self, name))
		return(false);
	else {
		char* absolute_name = ToUTF8(REPL_get_absolute_pathw(FromUTF8(name)));
		REPL_set_file_modified(self, false);
		REPL_set_current_environment_name(self, absolute_name);
		return(true);
	}
}
bool REPL_confirm_close(struct REPL* self);
void REPL_load(struct REPL* self) {
	if(self->B_file_modified) {
		if(!REPL_confirm_close(self))
			return;
	}
	std::wstring file_name;
	file_name = GetUsualOpenFileName(self->dialog);
	REPL_load_contents_by_name(self, ToUTF8(file_name));
}

static INT_PTR CALLBACK HandleAboutMessages(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


static void ScreenToClientRect(HWND hwnd, RECT& rect) {
	POINT p;
	p.x = rect.left;
	p.y = rect.top;
	ScreenToClient(hwnd, &p);
	rect.left = p.x;
	rect.top = p.y;

	p.x = rect.right;
	p.y = rect.bottom;
	ScreenToClient(hwnd, &p);
	rect.right = p.x;
	rect.bottom = p.y;
}
static INT_PTR CALLBACK HandleConfirmCloseDialogMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
	switch(message) {
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;
	case WM_COMMAND:
		EndDialog(dialog, wParam);
		return((INT_PTR) TRUE);
	}
	return((INT_PTR) FALSE);
}
bool REPL_confirm_close(struct REPL* self) {
	switch(DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONFIRM_CLOSE), self->dialog, HandleConfirmCloseDialogMessage)) {
	case IDYES:
		  return(REPL_save(self, FALSE));
	case IDNO:
		  return(true);
	default:
		  return(false);
	}
}
static void REPL_find_text(struct REPL* self, const std::wstring& text, bool upwards, bool case_sensitive) {
	int index;
	FINDTEXTEXW range;
	if(upwards) {
		range.chrg.cpMin = GetRichTextCaretPosition(GetDlgItem(self->dialog, IDC_OUTPUT)); // FIXME curstart, W;
		range.chrg.cpMax = 0;
	} else {
		range.chrg.cpMin = 0;
		range.chrg.cpMax = -1;
	}
	range.chrgText.cpMax = -1;
	range.chrgText.cpMin = -1;
	range.lpstrText = (WCHAR*) text.c_str();
	index = SendMessage(GetDlgItem(self->dialog, IDC_OUTPUT), EM_FINDTEXTEXW, (!upwards ? FR_DOWN : 0) | (case_sensitive ? FR_MATCHCASE : 0), (LPARAM) &range);
	if(index != -1 && range.chrgText.cpMax != -1 && range.chrgText.cpMin != -1) { // found
		SendMessage(GetDlgItem(self->dialog, IDC_OUTPUT), EM_SETSEL, (WPARAM) range.chrgText.cpMin,(LPARAM) range.chrgText.cpMax);
	}
	// TODO search in selection?
	// TODO FR_WHOLEWORD
	//LPCTSTR   lpstrText;
	//CHARRANGE chrgText;
	// FIXME
}
INT_PTR CALLBACK HandleSearchDialogMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	struct REPL* self;
	self = (struct REPL*) GetWindowLongPtr(dialog, GWLP_USERDATA);
	switch (message) {
	case WM_INITDIALOG:
		SetDialogFocus(dialog, IDC_SEARCH_TERM);
		return (INT_PTR)FALSE;
	case WM_CLOSE:
	case WM_DESTROY:
		//EndDialog(dialog, IDCLOSE); /* or rather HideWindows */
		// save work.
		break;
	case WM_SIZE:
		break;
	case WM_COMMAND:
		if(LOWORD(wParam) == IDOK) {
			self->fBSearchUpwards = GetDlgItemCheckedCXX(self->fSearchDialog, IDC_SEARCH_UPWARDS);
			self->fBSearchCaseSensitive = GetDlgItemCheckedCXX(self->fSearchDialog, IDC_SEARCH_CASE_SENSITIVE);
			self->fSearchTerm = GetDlgItemTextCXX(self->fSearchDialog, IDC_SEARCH_TERM);
			REPL_find_text(self, self->fSearchTerm, self->fBSearchUpwards, self->fBSearchCaseSensitive);
		} else if(LOWORD(wParam) == IDCANCEL || LOWORD(wParam) == IDCLOSE) {
			ShowWindow(dialog, SW_HIDE);
		}
		break;
	}
	//return(DefDlgProc(dialog, message, wParam, lParam));
	return (INT_PTR)FALSE;
}
static void REPL_show_search_dialog(struct REPL* self) {
	//gtk_window_set_transient_for(GTK_WINDOW(dialog), self->fWidget);
	{
		std::wstring searchTerm = GetDlgItemTextCXX(self->fSearchDialog, IDC_SEARCH_TERM);
		SetDlgItemCheckedCXX(self->fSearchDialog, IDC_SEARCH_UPWARDS, self->fBSearchUpwards);
		SetDlgItemCheckedCXX(self->fSearchDialog, IDC_SEARCH_CASE_SENSITIVE, self->fBSearchCaseSensitive);
		SetDlgItemTextCXX(self->fSearchDialog, IDC_SEARCH_TERM, self->fSearchTerm.c_str());
	}
	ShowWindow(self->fSearchDialog, SW_SHOW);
	//ShowWindow(self->fSearchDialog, SW_HIDE);
}
void REPL_handle_find_next(struct REPL* self) {
	REPL_find_text(self, self->fSearchTerm, self->fBSearchUpwards, self->fBSearchCaseSensitive);
}
void REPL_handle_find(struct REPL* self) {
	const char* text = NULL;
	REPL_show_search_dialog(self);
}
static void REPL_handle_environment_row_activation(struct REPL* self, HWND list, int index) {
	using namespace AST;
	char* command;
	bool B_ok = false;
	if(index > -1) {
		command = NULL;
		std::wstring name = GetListBoxEntryStringCXX(GetDlgItem(self->dialog, IDC_ENVIRONMENT), index);
		command = ToUTF8(name);
		if(!command)
			return;
		/* TODO ensure newline */
		std::stringstream sst;
		std::string escapedName = AST::intern(command)->str();
		sst << "(cons (quote define) (cons (quote " << escapedName << ") (cons " << escapedName << " nil)))";
		std::string commandStr = sst.str();
		//command = g_strdup_printf("(cons (quote define) (cons (quote %s) (cons %s nil)))", command, command);
		B_ok = REPL_execute(self, commandStr.c_str());
	}
	if(B_ok)
		SendMessageW(list, LB_SETCURSEL, (WPARAM) -1, 0);
}
static LRESULT CALLBACK HandleEditTabMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
	struct REPL* self;
	self = (struct REPL*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch(message) {
	case WM_GETDLGCODE:
		return(DLGC_WANTALLKEYS | CallWindowProc(self->oldEditBoxProc, hwnd, message, wParam, lParam));
	case WM_CHAR:
		if(wParam == VK_TAB)
			return(0);
		else
			break;
	case WM_KEYDOWN:
		if(wParam == VK_TAB) {
			Completer_complete(self->fCompleter);
			return(0);
		}
	case WM_KEYUP:
		if(wParam == VK_TAB)
			return(0);
	}
	return(CallWindowProc(self->oldEditBoxProc, hwnd, message, wParam, lParam));
}
INT_PTR CALLBACK HandleREPLMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	struct REPL* self;
	self = (struct REPL*) GetWindowLongPtr(dialog, GWLP_USERDATA);
	switch (message) {
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(dialog, IDC_OUTPUT), EM_SETEVENTMASK, 0, SendMessage(GetDlgItem(dialog, IDC_OUTPUT), EM_GETEVENTMASK, 0, 0) | ENM_CHANGE);
		SetDialogFocus(dialog, IDC_COMMAND_ENTRY);
		return (INT_PTR)FALSE;
	case WM_CLOSE:
	case WM_DESTROY:
		//EndDialog(dialog, IDCLOSE); /* or rather HideWindows */
		// save work.
		if(!self->B_file_modified || REPL_confirm_close(self)) {
			{
				RECT rect;
				GetWindowRect(dialog, &rect);
				int height = rect.bottom - rect.top;
				int width = rect.right - rect.left;
				Config_set_main_window_width(self->fConfig, width);
				Config_set_main_window_height(self->fConfig, height);
			}
			Config_save(self->fConfig);
			PostQuitMessage(0);
		}
		break;
	/*case WM_GETDLGCODE:
		{
			if(wParam) {
				printf("WPARAM %d\n", (int) wParam);
			}
			if(lParam) {
				LPMSG msg = (LPMSG) lParam;
				printf("%d\n", msg->message);
			}
			return(DLGC_WANTALLKEYS); //TAB);
		}*/
	case WM_SIZE:
		{
			RECT clientRect;
			RECT executeButtonRect;
			RECT windowRect;
			RECT outputRect;
			RECT commandEntryRect;
			RECT environmentRect;
			GetClientRect(dialog, &clientRect);
			int cx = clientRect.right - clientRect.left;
			int cy = clientRect.bottom - clientRect.top;
			int cxtot = cx;
			GetWindowRect(GetDlgItem(dialog, IDC_EXECUTE), &executeButtonRect);
			ScreenToClientRect(dialog, executeButtonRect);
			GetWindowRect(GetDlgItem(dialog, IDC_OUTPUT), &outputRect);
			ScreenToClientRect(dialog, outputRect);
			GetWindowRect(GetDlgItem(dialog, IDC_COMMAND_ENTRY), &commandEntryRect);
			ScreenToClientRect(dialog, commandEntryRect);
			GetWindowRect(GetDlgItem(dialog, IDC_ENVIRONMENT), &environmentRect);
			ScreenToClientRect(dialog, environmentRect);
			int commandEntryHeight = commandEntryRect.bottom - commandEntryRect.top;
			int executeButtonWidth = executeButtonRect.right - executeButtonRect.left;
			//cx -= outputRect.left + 10;
			cy -= outputRect.top + commandEntryHeight + 14;
			//MoveWindow(,,,, , FALSE);
			SetWindowPos(GetDlgItem(dialog, IDC_EXECUTE), NULL, cx - executeButtonWidth - 10, cy + 20, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(dialog, IDC_OUTPUT), NULL, 0, 0, cx - outputRect.left - 10, cy, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(dialog, IDC_ENVIRONMENT), NULL, 0, 0, environmentRect.right - environmentRect.left, cy, SWP_NOMOVE|SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			SetWindowPos(GetDlgItem(dialog, IDC_COMMAND_ENTRY), NULL, 10, cy + 20, cx - 20 - executeButtonWidth, commandEntryRect.bottom - commandEntryRect.top, SWP_NOACTIVATE|SWP_NOREPOSITION|SWP_NOZORDER);
			//GetDlgItem(self->dialog, IDC_OUTPUT)
		}
		break;
		/*case WM_GETMINMAXINFO:
      lppt = (LPPOINT)lParam;   // lParam points to array of POINTs
      lppt[3].x = MinDialogSize.x;
      lppt[3].y = MinDialogSize.y;
      lppt[4].x = MaxDialogSize.x;
      lppt[4].y = MaxDialogSize.y;
      return (INT_PTR)FALSE;
      break;*/

	case WM_NEXTDLGCTL:
		{
			if(self && self->dialog)
				ClearRichTextSelection(GetDlgItem(self->dialog, IDC_OUTPUT));
		}
		break;
	case WM_COMMAND:
		switch(LOWORD(wParam)) {
		case IDM_FILE_SAVE:
				REPL_save(self, FALSE);
				break;
		case IDM_FILE_SAVE_AS:
				REPL_save(self, TRUE);
				break;
		case IDM_ABOUT:
			DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUTBOX), dialog, HandleAboutMessages);
			break;
		case IDM_EXIT:
			DestroyWindow(dialog);
			break;
		case IDM_FILE_OPEN:
				REPL_load(self);
				break;
		case IDC_EXECUTE:
		case IDM_FILE_EXECUTE:
			{
				std::wstring text;
				bool B_used_entry = false;
				HWND output;
				output = GetDlgItem(self->dialog, IDC_OUTPUT);
				text = GetRichTextSelectedText(output);
				if(text.length() == 0) {
					text = GetDlgItemTextCXX(self->dialog, IDC_COMMAND_ENTRY);
					B_used_entry = true;
					REPL_append_to_output_buffer(self, ToUTF8(text));
				}
				std::string UTF8_text = ToUTF8(text);
				try {
					REPL_execute(self, UTF8_text.c_str());
					if(B_used_entry) {
						SetDlgItemTextCXX(self->dialog, IDC_COMMAND_ENTRY, _T(""));
					}
				} catch(...) {
				}
				SetDialogFocus(self->dialog, IDC_COMMAND_ENTRY);
				break;
			}
		case IDM_EDIT_CUT:
		case IDM_EDIT_COPY:
		case IDM_EDIT_PASTE:
			{
				SendMessage(GetFocus(), LOWORD(wParam) == IDM_EDIT_CUT ? WM_CUT :
					                    LOWORD(wParam) == IDM_EDIT_COPY ? WM_COPY :
										LOWORD(wParam) == IDM_EDIT_PASTE ? WM_PASTE : WM_CLEAR, 0, 0);
				break;
			}
		case IDC_OUTPUT:
			{
				if(HIWORD(wParam) == EN_CHANGE)
					REPL_set_file_modified(self, true);
				break;
			}
		case IDC_ENVIRONMENT:
			{
				if(HIWORD(wParam) == LBN_DBLCLK) {
					HWND list = GetDlgItem(self->dialog, IDC_ENVIRONMENT);
					int rowIndex = (int) SendMessage((HWND)lParam, LB_GETCURSEL, 0,0);
					if(rowIndex == LB_ERR)
						rowIndex = -1;
					REPL_handle_environment_row_activation(self, list, rowIndex);
				}
				break;
			}
		case IDM_EDIT_FIND:
			{
				REPL_handle_find(self);
				break;
			}
		case IDM_EDIT_FINDNEXT:
			{
				REPL_handle_find_next(self);
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
static LRESULT CALLBACK HandleREPLMessage2(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return(::DefWindowProc(hwnd, msg, wParam, lParam));
}
static ATOM registerMyClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS | CS_NOCLOSE | /*CS_OWNDC |*/ CS_PARENTDC;
	wcex.lpfnWndProc	= HandleREPLMessage2;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	//wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY5D));
	//wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	//wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MY5D);
	wcex.lpszClassName	= _T("REPL");
	//wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
static void REPL_init_builtins(struct REPL* self);
void REPL_init(struct REPL* self, HWND parent) {
	HINSTANCE hinstance;
	self->fEnvironmentKeys = new std::set<AST::Symbol*>;
	self->fBSearchUpwards = true;
	self->fBSearchCaseSensitive = true;
	hinstance = GetModuleHandle(NULL);
	//DialogBox(hinstance, MAKEINTRESOURCE(IDD_REPL), parent, HandleREPLMessage);
	self->B_file_modified = false;
	/*if(!registerMyClass(hinstance)) {
		ShowWIN32Diagnostics();
	}*/
	self->dialog = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_REPL), parent, HandleREPLMessage);
	if(self->dialog == NULL) {
		ShowWIN32Diagnostics();
	}
	self->oldEditBoxProc = (WNDPROC) SetWindowLong(GetDlgItem(self->dialog, IDC_COMMAND_ENTRY), GWL_WNDPROC, (LONG) HandleEditTabMessage);
	self->fCompleter = Completer_new(GetDlgItem(self->dialog, IDC_COMMAND_ENTRY), self->fEnvironmentKeys);
	SetWindowLongPtr(GetDlgItem(self->dialog, IDC_COMMAND_ENTRY), GWLP_USERDATA, (LONG) self);
	SetWindowLongPtr(self->dialog, GWLP_USERDATA, (LONG) self);
	self->fSearchDialog = CreateDialog(hinstance, MAKEINTRESOURCE(IDD_SEARCH), self->dialog, HandleSearchDialogMessage);
	if(self->fSearchDialog == NULL) {
		ShowWIN32Diagnostics();
	}
	SetWindowLongPtr(self->fSearchDialog, GWLP_USERDATA, (LONG) self);
	//self->accelerators = LoadAccelerators(hinstance, MAKEINTRESOURCE(IDC_MY5D));
	REPL_init_builtins(self);
	REPL_set_file_modified(self, false);

	self->fConfig = load_Config();
	{
		char* environment_name;
		environment_name = Config_get_environment_name(self->fConfig);
		if(environment_name && environment_name[0])
			REPL_load_contents_by_name(self, environment_name);
	}
	{
		int width = Config_get_main_window_width(self->fConfig);
		int height = Config_get_main_window_height(self->fConfig);
		SetWindowPos(self->dialog, NULL, 0, 0, width, height, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);
	}
}
bool REPL_get_file_modified(struct REPL* self) {
	return(self->B_file_modified);
}
void REPL_add_to_environment_simple_GUI(struct REPL* self, struct AST::Symbol* parameter, struct AST::Node* value) {
	//std::string bodyString = body->str();
	if(self->fEnvironmentKeys->find(parameter) == self->fEnvironmentKeys->end()) {
		EnsureInEnvironment(self->dialog, FromUTF8(parameter->name));
		self->fEnvironmentKeys->insert(parameter);
		REPL_set_file_modified(self, true);
	}
}
static AST::Node* REPL_close_environment(struct REPL* self, AST::Node* node);
/* TODO abstract into common place */
bool REPL_execute(struct REPL* self, const char* command) {
	Scanners::MathParser parser;
	FILE* input_file = fmemopen((void*) command, strlen(command), "r");
	if(input_file) {
		try {
			try {
				parser.push(input_file, 0);
				AST::Node* result = parser.parse();
				REPL_add_to_environment(self, result);
				if(!result || dynamic_cast<AST::Cons*>(result) == NULL || ((AST::Cons*)result)->head != AST::intern("define")) {
					result = REPL_close_environment(self, result);
					result = Evaluators::provide_dynamic_builtins(result);
					result = Evaluators::annotate(result);
					result = Evaluators::reduce(result);
				}
				//result = Evaluators::annotate(result);
				std::string v = result ? result->str() : "OK";
				v = " => " + v + "\n";
				REPL_append_to_output_buffer(self, v.c_str());
			} catch(...) {
				fclose(input_file);
				throw;
			}
		} catch(Scanners::ParseException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			REPL_append_to_output_buffer(self, v.c_str());
			REPL_set_file_modified(self, true);
			throw;
		} catch(Evaluators::EvaluationException e) {
			std::string v = e.what() ? e.what() : "error";
			v = " => " + v + "\n";
			REPL_append_to_output_buffer(self, v.c_str());
			REPL_set_file_modified(self, true);
			throw;
		}
		REPL_set_file_modified(self, true);
	}
}
void REPL_append_to_output_buffer(struct REPL* self, const char* text) {
	InsertRichText(GetDlgItem(self->dialog, IDC_OUTPUT), FromUTF8(text));
}
char* REPL_get_output_buffer_text(struct REPL* self) {
	std::wstring value = GetDlgItemTextCXX(self->dialog, IDC_OUTPUT);
	return(ToUTF8(value));
}
void REPL_clear(struct REPL* self) {
	self->fTailEnvironment = self->fTailUserEnvironment = self->fTailUserEnvironmentFrontier = NULL;
	SetDlgItemTextCXX(self->dialog, IDC_OUTPUT, _T(""));
	while(SendDlgItemMessageW(self->dialog, IDC_ENVIRONMENT, LB_DELETESTRING, 0, 0) > 0)
		;
	// or just LB_RESETCONTENT
	self->fEnvironmentKeys->clear();
	REPL_init_builtins(self);
	REPL_set_file_modified(self, false);
}
static AST::Cons* box_environment_elements(HWND dialog, int index, int count) {
	if(index >= count)
		return(NULL);
	else {
		// FIXME Ptr
		AST::Node* value = (AST::Node*) SendDlgItemMessageW(dialog, IDC_ENVIRONMENT, LB_GETITEMDATA, (WPARAM) index, (LPARAM) 0);
		std::wstring name = GetListBoxEntryStringCXX(GetDlgItem(dialog, IDC_ENVIRONMENT), index);
		AST::Symbol* nameSymbol = AST::intern(ToUTF8(name));
		return(cons(cons(nameSymbol, cons(value, NULL)), box_environment_elements(dialog, index + 1, count)));
	}
}
AST::Cons* REPL_get_environment(struct REPL* self) {
	int count = SendDlgItemMessageW(self->dialog, IDC_ENVIRONMENT, LB_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
	return(box_environment_elements(self->dialog, 0, count));
}
PHANDLE REPL_get_waiting_handles(struct REPL* REPL, DWORD* handleCount) {
	*handleCount = 0;
	return(NULL);
}

}; // end namespace GUI

#include "GUI/REPLEnvironment"
