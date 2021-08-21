#include "wfilesearcher.hpp"
#include <string>
#include <vector>

static const wchar_t * wfsmainclassname = L"WFSEARCHMAIN";
static HINSTANCE appInstance = 0;
static ATOM mainatom = 0;
extern int quitCode;

static HWND mainwin;
static std::wstring currSearchDir;

static BOOL GetFullPathNameAsWString(std::wstring * dst, const std::wstring &pathname) {
  DWORD nbytes = GetFullPathNameW(pathname.c_str(), 0, 0, 0);
  wchar_t * pathbuf;
  BOOL success = FALSE;
  if(nbytes) {
    DWORD pathstrlen;
    pathbuf = new wchar_t[nbytes];
    if((pathstrlen = GetFullPathNameW(pathname.c_str(), nbytes, pathbuf, 0))) {
      dst -> resize(pathstrlen);
      for(DWORD i = 0; i < pathstrlen; i += 1)
        (*dst)[i] = pathbuf[i];
      success = TRUE;
    }
    delete[] pathbuf;
  }
  return success;
}

static BOOL ListDirectory(std::vector<std::wstring> * dst, const std::wstring &dirstr) {
  std::vector<std::wstring> copydst = *dst;
  std::wstring findfiletarget = dirstr;
  HANDLE dhandle;
  WIN32_FIND_DATAW data;
  BOOL status = FALSE;
  if(findfiletarget.back() != '\\') {
    findfiletarget.push_back('\\');
  }
  findfiletarget.push_back('*');
  if((dhandle = FindFirstFileW(findfiletarget.c_str(), &data)) != INVALID_HANDLE_VALUE) {
    BOOL done = FALSE;
    while(!done) {
      if(wcscmp(L"..", data.cFileName) && wcscmp(L".", data.cFileName))
        copydst.push_back(std::wstring(data.cFileName));
      if(!FindNextFileW(dhandle, &data)) {
        done = TRUE;
        status = (GetLastError() == ERROR_NO_MORE_FILES);
        FindClose(dhandle);
      }
    }
  }
  if(status)
    *dst = copydst;
  return status;
}

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
  HWND hwnd;
  if(!(hwnd = CreateWindowExW(
                  exstyles,
                  wfsmainclassname,
                  name,
                  styles,
                  x, y, w, h,
                  0, 0, 
                  appInstance, 0))) {
    return 0;
  }
  return hwnd;
}

static HWND CreateWFSearchMainWindow() {
  return CreateWFSearchWindow(
    (wchar_t *)"Wide File Searcher", 
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



static const wchar_t * InitializeCurrSearchDir() {
  if(! GetFullPathNameAsWString(&currSearchDir, std::wstring(L".")))
    return 0;
  return currSearchDir.c_str();
}

static void PrintDirectory() {
  std::vector<std::wstring> files;
  if(ListDirectory(&files, currSearchDir)) {
    for(int i = 0; i < files.size(); i += 1)
      printf("%ls\n", files[i].c_str());
  }
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
    return FALSE;
  }
  PrintDirectory();
  ShowWFSearchWindow(mainwin);
  return TRUE;
}