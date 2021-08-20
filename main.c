#include "enumfiles.h"

int quitCode = 0;

int main() {
  WFSFILEENUM * enumerator = CreateFileEnumerator(L"C:\\Users\\Moose");
  DWORD attr;
  wchar_t * name = NextFileName(enumerator, &attr, 0);
  while(name) {
    printf("%ls (Directory: %s)\n", name, (attr & FILE_ATTRIBUTE_DIRECTORY) ? "True" : "False");
    name = NextFileName(enumerator, &attr, 0);
  }
  return quitCode;
}