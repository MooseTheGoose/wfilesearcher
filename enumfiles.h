#ifndef ENUM_FILES_H
#define ENUM_FILES_H

#include <Windows.h>

typedef SIZE_T size_t;
typedef void * HANDLE;

typedef struct _dirobj {
  struct _dirobj * breadth;
  struct _dirobj * parent;
  wchar_t name[]; 
} WFSDIROBJECT;

typedef struct _fpath {
  wchar_t * fpath;
  size_t baselen;
  size_t len;
  size_t alloclen;
} WFSFILEPATH;

/*
 *  Consier putting a "level" member
 *  in this structure so that file enumeration
 *  can stop after a certain level.
 */
typedef struct _fenumerator {
  HANDLE dhandle;
  WIN32_FIND_DATAW result;
  INT done;
  WFSFILEPATH * currpath;
  WFSDIROBJECT * root;
  WFSDIROBJECT * current;
  WFSDIROBJECT ** next;
  WFSDIROBJECT * end;
} WFSFILEENUM;

WFSFILEPATH * CreateWfsFilePath(const wchar_t * fullbase);
WFSDIROBJECT * CreateDirObject(const wchar_t * name);
WFSFILEENUM * CreateFileEnumerator(const wchar_t * path);
wchar_t * PreparePathAppend(WFSFILEPATH * path, size_t nbytes);
wchar_t * ConstructPathFromDir(WFSFILEPATH * path, const WFSDIROBJECT * dir);
void DestroyWfsFilePath(WFSFILEPATH * path);
void DestroyDirObjectRoot(WFSDIROBJECT * root);
void DestroyFileEnumerator(WFSFILEENUM * enumerator);
wchar_t * AllocFullPathName(const wchar_t * pathname);
wchar_t * NextFileName(WFSFILEENUM * enumerator, DWORD * attributes, const wchar_t ** dirstr);

#endif