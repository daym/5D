// 5D.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <tchar.h>
#include <string.h>
#include <sstream>
#include "resource.h"
#include <stdio.h>
#include <shellapi.h>
#include "Commctrl.h"
#include "GUI/WIN32REPL"
#include "Evaluators/FFI"
#include "REPL/REPL"
#include "AST/AST"

#define MAX_LOADSTRING 100

static struct REPLX::REPL* REPL1;

// Global Variables:
static HINSTANCE hInst;								// current instance
static TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
//static TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   //HWND hWnd;
   using namespace GUI;

   hInst = hInstance; // Store instance handle in our global variable

   InitCommonControls();
   {INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
   }
   GC_INIT();
   //GC_disable();
   LoadLibrary(_T("RICHED32.DLL"));
   REPL1 = GUI::REPL_new(NULL);
   ShowWindow(REPL_get_window(REPL1), nCmdShow);
   UpdateWindow(REPL_get_window(REPL1));
   return TRUE;
}

/*void Repaint(HWND hwnd, PAINTSTRUCT* ps) {
}*/

using namespace GUI;

//typedef std::basic_stringstream<wchar_t> wstringstream;

static void handleCommandLine(void) {
	int argc = 0;
	LPWSTR commandLine = GetCommandLineW();
#if 0
	/* I don't feel like working around shell bugs. */
	int rc;
	WCHAR moduleFileName[2049];
	rc = GetModuleFileNameW(NULL, moduleFileName, 2048);
	if(!(rc >= 2048 || rc == 0)) { /* worked */
		if(_wstrcmp(moduleFileName, commandLine) == 0) { /* Vista passes the command line without quotes sometimes. Don't ask. */
			/* no arguments there. */
			return;
		}
	}
#endif
	LPWSTR* argv = CommandLineToArgvW(commandLine, &argc);
	if(argv == NULL) // ???
		return;
	if(argc > 1) {
		//MessageBox(NULL, argv[1], NULL, MB_OK);
		if(!REPL_load_contents_by_name(REPL1, ToUTF8(argv[1]))) {
			std::wstring s = _T("could not load \"");
			s += argv[1];
			s += std::wstring(_T("\""));
			MessageBox(NULL, s.c_str(), NULL, MB_OK);
		}
	}
	LocalFree(argv);
}
#ifdef _MSC_VER
int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow) {
#else
extern "C"
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#endif
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	MSG msg;
	HACCEL hAccelTable;
	WCHAR buffer[2049];
	if(GetModuleFileNameW(GetModuleHandle(NULL), buffer, 2048) > 0) {
		char* name = Evaluators::get_absolute_path(ToUTF8(buffer));
		char* slash = strrchr(name, '\\');
		std::string name2;
		if(slash != NULL) {
			*(slash + 1) = 0;
			name2 = name;
		} else {
			name2 = name;
			name2 += "\\";
		}
		name2 += "..\\share\\";
		GUI::REPL_set_shared_dir(name2);
	}
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_MY5D, szWindowClass, MAX_LOADSTRING);
	if (!InitInstance(hInstance, nCmdShow))
		return(FALSE);
	handleCommandLine();
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY5D));
	DWORD reason = WAIT_TIMEOUT;
	while(1) {
		DWORD handleCount = 0;
		PHANDLE handles = REPL1 ? REPL_get_waiting_handles(REPL1, &handleCount) : NULL;
		reason = MsgWaitForMultipleObjects(handleCount, handles, FALSE, INFINITE, QS_ALLEVENTS);
		if(reason >= WAIT_OBJECT_0 && reason < WAIT_OBJECT_0 + handleCount) {
			/* notify waiter of handle */
			/*DWORD dwExitCode;
			 if ( ! ::GetExitCodeProcess( hProcess, &dwExitCode)||dwExitCode!=STILL_ACTIVE)
			childHandle = NULL;*/
			PostMessage(REPL_get_window(REPL1), FM_NOTIFY_SYSTEM, (WPARAM) handles[reason - WAIT_OBJECT_0], 0);
		} else { /* WAIT_OBJECT_0 + x */
			while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
				if(msg.message == WM_QUIT)
					return((int) msg.wParam);
				// TODO IsWindow ? 
				if (!REPL1 || 
					((!TranslateAccelerator(REPL_get_window(REPL1), hAccelTable, &msg) && !IsDialogMessage(REPL_get_window(REPL1), &msg)) &&
					(!TranslateAccelerator(REPL_get_search_window(REPL1), hAccelTable, &msg) && !IsDialogMessage(REPL_get_search_window(REPL1), &msg)))) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
	}
	return((int) msg.wParam);
}

