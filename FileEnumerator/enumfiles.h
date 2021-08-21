#ifndef ENUM_FILES_H
#define ENUM_FILES_H

#include <Windows.h>
#include <stdint.h>

#define DIROBJ_ALLOCATOR_TABLE_SIZE (sizeof(size_t) * 8) 
#define GETNEXTFILE_FLAG_DONE 0x1
#define GETNEXTFILE_FLAG_DIRCHANGED 0x2 

typedef struct _dirobj {
  struct _dirobj * breadth;
  struct _dirobj * parent;
  wchar_t name[]; 
} WFSDIROBJECT;

typedef struct _dirobjalloc {
  uint8_t * alloctable[DIROBJ_ALLOCATOR_TABLE_SIZE];
  size_t currtablepos;
  size_t currtablelimit;
  size_t currtableidx;
} WFSDIROBJECTALLOCATOR;

typedef struct _fpath {
  wchar_t * fpath;
  size_t baselen;
  size_t len;
  size_t alloclen;
} WFSFILEPATH;

/*
 *  File enumerator. Give it either a file
 *  or directory, and it will list everything
 *  recursively one by one.
 *
 *  It can be used in general, but it's overkill
 *  for checking on files and directories without
 *  recursing. There's also a lot of moving parts,
 *  so it's very hard to diagnose when things go wrong.
 *  That's why you're likely better off just using
 *  the Win32 functions in that case.
 */
typedef struct _fenumerator {
  WIN32_FIND_DATAW result;
  HANDLE dhandle;
  BOOL done;
  DWORD minlevel;
  DWORD maxlevel;
  DWORD currlevel;
  wchar_t * relativedir;
  WFSFILEPATH * currpath;
  WFSDIROBJECTALLOCATOR * allocator;
  WFSDIROBJECT * root;
  WFSDIROBJECT * current;
  WFSDIROBJECT ** next;
  WFSDIROBJECT * end;
} WFSFILEENUM;

WFSFILEPATH * CreateWfsFilePath(const wchar_t * fullbase);
WFSFILEENUM * CreateFileEnumerator(const wchar_t * path);
WFSDIROBJECTALLOCATOR * CreateDirObjectAllocator();
WFSDIROBJECT * AllocateDirObjectFromWfs(WFSDIROBJECTALLOCATOR * allocator, const wchar_t * name);
wchar_t * PreparePathAppend(WFSFILEPATH * path, size_t nbytes);
wchar_t * ConstructPathFromDir(WFSFILEPATH * path, const WFSDIROBJECT * dir, DWORD * level);
void DestroyWfsFilePath(WFSFILEPATH * path);
void DestroyFileEnumerator(WFSFILEENUM * enumerator);
void DestroyDirObjectAllocator(WFSDIROBJECTALLOCATOR * allocator);
DWORD GetNextFileFromEnumerator(WFSFILEENUM * enumerator);

/* Shouldn't need these, but just in case */
WFSDIROBJECT * CreateDirObject(const wchar_t * name);
void DestroyDirObjectRoot(WFSDIROBJECT * root);

#endif