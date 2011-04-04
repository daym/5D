// 4D.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include <tchar.h>
#include "resource.h"
#include <stdio.h>
#include "Commctrl.h"
#include "GUI/WIN32REPL"

#define MAX_LOADSTRING 100

static struct GUI::REPL* REPL;

// Global Variables:
static HINSTANCE hInst;								// current instance
static TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
//static TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

static BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   InitCommonControls();
   {INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
   }
   LoadLibrary(_T("RICHED32.DLL"));
   REPL = GUI::REPL_new(NULL);
   ShowWindow(REPL_get_window(REPL), nCmdShow);
   UpdateWindow(REPL_get_window(REPL));
   return TRUE;
}

/*void Repaint(HWND hwnd, PAINTSTRUCT* ps) {
}*/

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
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	//LoadString(hInstance, IDC_MY4D, szWindowClass, MAX_LOADSTRING);
	if (!InitInstance(hInstance, nCmdShow))
		return(FALSE);
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MY4D));
	while(GetMessage(&msg, NULL, 0, 0)) {
		// TODO IsWindow ? 
		if (!REPL || (!TranslateAccelerator(REPL_get_window(REPL), hAccelTable, &msg) && !IsDialogMessage(REPL_get_window(REPL), &msg))) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return((int) msg.wParam);
}

