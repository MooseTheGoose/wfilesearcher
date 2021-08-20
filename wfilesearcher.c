#include "wfilesearcher.h"
#include "enumfiles.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const wchar_t * wfsmainclassname = L"WFSEARCHMAIN";
static HINSTANCE appInstance = 0;
static ATOM mainatom = 0;
extern int quitCode;

static HWND mainwin;
static wchar_t * currSearchDir;

typedef struct _wfsfileinfo {
  struct _wfsfileinfo * next;
  wchar_t data[];
} WFSFILEINFO;

static LRESULT MainWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
  switch(msg) {
    case WM_DESTROY: {
      PostQuitMessage(0);
      return 0;
    }
    case WM_PAINT: {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW+1));
      EndPaint(hwnd, &ps);
      return 0;
    }
  }
  return DefWindowProc(hwnd, msg, wparam, lparam);
}

static HWND CreateWFSearchWindow(const wchar_t * name, int x, int y, int w, int h, DWORD styles, DWORD exstyles) {
  wchar_t * newname;
  HWND hwnd;
  if(!(newname = _wcsdup(name)))
    return 0;
  if(!(hwnd = CreateWindowExW(
                  exstyles,
                  wfsmainclassname,
                  name,
                  styles,
                  x, y, w, h,
                  0, 0, 
                  appInstance, 0))) {
    free(newname);
    return 0;
  }
  return hwnd;
}

static HWND CreateWFSearchMainWindow() {
  return CreateWFSearchWindow(
    L"Wide File Searcher", 
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    WS_OVERLAPPEDWINDOW, 0);
}

static void ShowWFSearchWindow(HWND win) {
  ShowWindow(win, SW_SHOW);
}


BOOL WFSearchHandleEvents() {
  MSG msg;

  memset(&msg, 0, sizeof(msg));
  while(PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) {
    if(msg.message == WM_QUIT) {
      quitCode = msg.wParam;
      return FALSE;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return TRUE;
}

static wchar_t * InitializeCurrSearchDir() {
  wchar_t * currSearchDir = AllocFullPathName(L".");
  if(! currSearchDir) {
    currSearchDir = _wcsdup(L"C:\\");
  }
  return currSearchDir;
}

BOOL WFSearchInit() {
  WNDCLASSW mainclass;

  appInstance = GetModuleHandleW(0);
  memset(&mainclass, 0, sizeof(mainclass));
  mainclass.style = 0;
  mainclass.hInstance = appInstance;
  mainclass.lpszClassName = wfsmainclassname;
  mainclass.lpfnWndProc = MainWindowProc;
  if(!(mainatom = RegisterClassW(&mainclass)))
    return FALSE;
  if(! InitializeCurrSearchDir())
    return FALSE;
  if(!(mainwin = CreateWFSearchMainWindow())) {
    free(currSearchDir);
    return FALSE;
  }
  ShowWFSearchWindow(mainwin);
  return TRUE;
}