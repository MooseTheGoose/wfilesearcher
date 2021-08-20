#ifndef ENUM_FILES_H
#define ENUM_FILES_H

#include <Windows.h>
#include <stdint.h>

#define DIROBJ_ALLOCATOR_TABLE_SIZE (sizeof(size_t) * 8) 
#define GETNEXTFILE_FLAG_DONE 0x1
#define GETNEXTFILE_FLAG_DIRCHANGED 0x2 

typedef void * HANDLE;

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
 *  Consider putting a "level" member
 *  in this structure so that file enumeration
 *  can stop after a certain level.
 */
typedef struct _fenumerator {
  HANDLE dhandle;
  WIN32_FIND_DATAW result;
  BOOL done;
  DWORD minlevel;
  DWORD maxlevel;
  DWORD currlevel;
  wchar_t * relativedir;
  WFSFILEPATH * currpath;
  WFSDIROBJECTALLOCATOR * allocator;
  WFSDIROBJECT * firstInLevel;
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
wchar_t * AllocFullPathName(const wchar_t * pathname);
DWORD GetNextFileFromEnumerator(WFSFILEENUM * enumerator);

/* Shouldn't need these, but just in case */
WFSDIROBJECT * CreateDirObject(const wchar_t * name);
void DestroyDirObjectRoot(WFSDIROBJECT * root);

#endif