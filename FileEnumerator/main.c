#include "enumfiles.h"
#include <errno.h>
#include <psapi.h>
int quitCode = 0;

void mainproc() {
  WFSFILEENUM * fenum = CreateFileEnumerator(L"C:\\");
  wchar_t * relpath;
  DWORD status = GetNextFileFromEnumerator(fenum);
  relpath = fenum -> relativedir;
  errno = 0;
  while(!(status & GETNEXTFILE_FLAG_DONE)) {
    status = GetNextFileFromEnumerator(fenum);
    if(status & GETNEXTFILE_FLAG_DIRCHANGED)
      relpath = fenum -> relativedir;
      printf("%ls\\%ls%ls\n", L"C:", relpath, fenum -> result.cFileName);
  }
  DestroyFileEnumerator(fenum);
}

int main(int argc, char ** argv) {
/*
  DWORD handlecount = 0;
  PROCESS_MEMORY_COUNTERS pmc;
  printf("Check Resource monitor\n");
  getch();
  printf("Doing stress test\n");
  for(int i = 0; i < 50; i += 1) {
    if(!GetProcessHandleCount(GetCurrentProcess(), &handlecount))
      handlecount = 0;
    if(GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
      printf("Page Fault Count: 0x%08x\n", pmc.PageFaultCount);
      printf("Peak Working Set Size: 0x%08x\n", pmc.PeakWorkingSetSize);
      printf("Quota Peak Paged Pool Usage: 0x%08x\n", pmc.QuotaPeakPagedPoolUsage);
      printf("Quota Paged Pool Usage: 0x%08x\n", pmc.QuotaPagedPoolUsage);
      printf("Quota Peak Non Paged Pool Usage: 0x%08x\n", pmc.QuotaPeakNonPagedPoolUsage);
      printf("Quota Non Paged Pool Usage: 0x%08x\n", pmc.QuotaNonPagedPoolUsage);
      printf("Page File Usage: 0x%08x\n", pmc.PagefileUsage);
      printf("Peak Page File Usage: 0x%08x\n", pmc.PeakPagefileUsage);
    }
    printf("i = %d, nhandles = %d\n", i, handlecount);
    mainproc();
  }
  printf("Check Resource monitor again\n");
  getch();
*/
  mainproc();
  return 0;
}