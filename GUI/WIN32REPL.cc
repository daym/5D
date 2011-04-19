#include <string>
#include <stdio.h>
#include "stdafx.h"
#include "resource.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <Commdlg.h>
#include "WIN32REPL"
#include "Scanners/MathParser"
#include "AST/AST"
#include "Evaluators/Evaluators"
#include "Config/Config"

namespace GUI {

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
static void initializeOpenFileNameStruct() {
	ZeroMemory(&openFileName, sizeof(openFileName));
	openFileName.lStructSize = sizeof(openFileName);
	openFileName.lpstrFile = openFileNameName;
	openFileName.lpstrFile[0] = '\0';
	openFileName.nMaxFile = sizeof(openFileNameName) / sizeof(openFileNameName[0]);
	openFileName.lpstrFilter = _T("All Files (*)\0*.*\04D Source Files (*.4D)\0*.4D\0\0");
	openFileName.nFilterIndex = 2;
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
std::wstring SetDlgItemTextCXX(HWND dialog, int control, WCHAR* buffer) {
	SetDlgItemTextW(dialog, control, buffer);
	// TODO error handling (if at all possible).
	return(buffer);
}
static void SetDialogFocus(HWND dialog, int control) {
	SendMessage(dialog, WM_NEXTDLGCTL, (WPARAM) GetDlgItem(dialog, control), (LPARAM) TRUE); 
}
/*BOOL LoadTextFileToEdit(HWND hEdit, LPCTSTR pszFileName)
{
    HANDLE hFile;
    BOOL bSuccess = FALSE;

    hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, 0, NULL);
    if(hFile != INVALID_HANDLE_VALUE) {
        DWORD dwFileSize;
        dwFileSize = GetFileSize(hFile, NULL);
        if(dwFileSize != 0xFFFFFFFF) {
            LPCWSTR pszFileText;
            pszFileText = (LPCWSTR) GlobalAlloc(GPTR, dwFileSize + 1);
            if(pszFileText != NULL) {
                DWORD dwRead;
                if(ReadFile(hFile, pszFileText, dwFileSize, &dwRead, NULL)) {
                    pszFileText[dwFileSize] = 0; // Add null terminator
                    if(SetWindowText(hEdit, pszFileText))
                        bSuccess = TRUE; // It worked!
                }
                GlobalFree(pszFileText);
            }
        }
        CloseHandle(hFile);
    }
    return bSuccess;
}
BOOL SaveTextFileFromEdit(HWND hEdit, LPCTSTR pszFileName)
{
    HANDLE hFile;
    BOOL bSuccess = FALSE;
    hFile = CreateFile(pszFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(hFile != INVALID_HANDLE_VALUE) {
        DWORD dwTextLength;
        dwTextLength = GetWindowTextLength(hEdit);
        // No need to bother if there's no text.
        if(dwTextLength > 0) {
            LPSTR pszText;
            DWORD dwBufferSize = dwTextLength + 1;
            pszText = GlobalAlloc(GPTR, dwBufferSize);
            if(pszText != NULL) {
                if(GetWindowText(hEdit, pszText, dwBufferSize)) {
                    DWORD dwWritten;
                    if(WriteFile(hFile, pszText, dwTextLength, &dwWritten, NULL))
                        bSuccess = TRUE;
                }
                GlobalFree(pszText);
            }
        }
        CloseHandle(hFile);
    }
    return bSuccess;
}*/

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
	//HACCEL accelerators;
};

HWND REPL_get_window(struct REPL* self) {
	return(self->dialog);
}
bool REPL_save(struct REPL* self) {
	std::wstring file_name;
	file_name = GetUsualSaveFileName(self->dialog);
	// TODO
	self->B_file_modified = false;
	return(true);
}
void REPL_load(struct REPL* self) {
	if(self->B_file_modified)
		if(!REPL_save(self))
			return;
	std::wstring file_name;
	file_name = GetUsualOpenFileName(self->dialog);
	// TODO
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
		  return(REPL_save(self));
	case IDNO:
		  return(true);
	default:
		  return(false);
	}
}
INT_PTR CALLBACK HandleREPLMessage(HWND dialog, UINT message, WPARAM wParam, LPARAM lParam) {
	UNREFERENCED_PARAMETER(lParam);
	struct REPL* self;
	self = (struct REPL*) GetWindowLongPtr(dialog, GWLP_USERDATA);
	switch (message) {
	case WM_INITDIALOG:
		SetDialogFocus(dialog, IDC_COMMAND_ENTRY);
		return (INT_PTR)FALSE;
	case WM_CLOSE:
	case WM_DESTROY:
		//EndDialog(dialog, IDCLOSE); /* or rather HideWindows */
		// save work.
		if(!self->B_file_modified || REPL_confirm_close(self))
				PostQuitMessage(0);
		break;

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
				REPL_save(self);
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
		case IDM_EDIT_FIND:
			/* FIXME implement */
			{
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
static ATOM registerMyClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style			= CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS | CS_NOCLOSE | /*CS_OWNDC |*/ CS_PARENTDC;
	wcex.lpfnWndProc	= ::DefWindowProc; // HandleREPLMessage2;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	//wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MY4D));
	//wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	//wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	//wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MY4D);
	wcex.lpszClassName	= _T("REPL");
	//wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}
void REPL_init(struct REPL* self, HWND parent) {
	HINSTANCE hinstance;
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
	//self->accelerators = LoadAccelerators(hinstance, MAKEINTRESOURCE(IDC_MY4D));
	SetWindowLongPtr(self->dialog, GWLP_USERDATA, (LONG) self);
}
void REPL_set_file_modified(struct REPL* self, bool value) {
	self->B_file_modified = value;
}
bool REPL_get_file_modified(struct REPL* self) {
	return(self->B_file_modified);
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
	REPL_set_file_modified(self, true);
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
				result = Evaluators::annotate(result);
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
			REPL_set_file_modified(self, true);
			throw;
		}
		REPL_set_file_modified(self, true);
	}
}
void REPL_set_current_environment_name(struct REPL* self, const char* absolute_name) {
	std::wstring text = FromUTF8(absolute_name);
	SetWindowText(self->dialog, text.c_str());
	Config_set_environment_name(self->fConfig, absolute_name);
	Config_save(self->fConfig);
}
char* REPL_get_absolute_path(const char* name) {
	return(strdup(name)); // FIXME
}
void REPL_append_to_output_buffer(struct REPL* self, const char* text) {
	// FIXME
}
char* REPL_get_output_buffer_text(struct REPL* self) {
	std::wstring value = GetDlgItemTextCXX(self->dialog, IDC_OUTPUT);
	return(ToUTF8(value));
}

}; // end namespace GUI
