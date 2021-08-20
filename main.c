#include "enumfiles.h"
#include <errno.h>
#include <stdio.h>

int quitCode = 0;

static wchar_t * convasciitowchar(char * ascii) {
  size_t len = strlen(ascii);
  if(len == SIZE_MAX)
    return 0;
  wchar_t * newstr = calloc(2, len + 1);
  if(!newstr)
    return 0;
  for(size_t i = 0; i < strlen(ascii); i += 1)
    newstr[i] = ascii[i];
  return newstr;
}

int main(int argc, char ** argv) {
  if(argc != 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    fprintf(stderr, "Print all the files recursively!\n");
    return 1;
  }
  errno = 0;
  DWORD status;
  wchar_t * fullpath = AllocFullPathName(convasciitowchar(argv[1]));
  wchar_t * dirpart;
  size_t fullpathlen = wcslen(fullpath);
  WFSFILEENUM * enumerator = CreateFileEnumerator(fullpath);
  WIN32_FIND_DATAW * data = &enumerator -> result;
  wchar_t * name = data -> cFileName;
  dirpart = enumerator -> relativedir;
  status = GetNextFileFromEnumerator(enumerator);
  if(fullpath[fullpathlen - 1] == '\\')
    fullpath[fullpathlen - 1] = 0;
  while(!(status & GETNEXTFILE_FLAG_DONE)) {
    if(status & GETNEXTFILE_FLAG_DIRCHANGED)
      dirpart = enumerator -> relativedir;
    if(errno == 0) {
      printf("%ls\\%ls%ls\n", fullpath, dirpart, name);
      errno = 0;
    }
    status = GetNextFileFromEnumerator(enumerator);
  }
  if(errno == 0) {
    fprintf(stderr, "Congrats! You made it!\n");
  } else if(errno == ENOMEM) {
    fprintf(stderr, "Wow. You actually ran out of memory... That's kind of hardcore.\n");
  } else {
    fprintf(stderr, "I really don't know what error you got... %d\n", errno);
  }
  DestroyFileEnumerator(enumerator);
  fprintf(stderr, "You destroyed that file enumerator like it was nothing!\n");
  return quitCode;
}