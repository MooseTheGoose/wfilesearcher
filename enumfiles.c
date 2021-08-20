#include "enumfiles.h"
#include "wfilesearcher.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include <stdio.h>

#define PATH_MIN_ALLOC 2048
#define DIROBJECTALLOC_MIN_ALLOC 0x100000

wchar_t *AllocFullPathName(const wchar_t *pathname) {
  DWORD pathsize = GetFullPathNameW(pathname, 0, 0, 0);
  wchar_t * fullpath = 0;
  if(pathsize) {
    DWORD status;
    fullpath = calloc(sizeof(wchar_t), pathsize);
    status = GetFullPathNameW(pathname, pathsize, fullpath, 0);
    if(! status) {
      free(fullpath);
      fullpath = 0;
    }
  }
  return fullpath;
}

WFSDIROBJECTALLOCATOR * CreateDirObjectAllocator() {
  WFSDIROBJECTALLOCATOR * allocator = calloc(1, sizeof(*allocator));
  if(allocator) {
    allocator -> alloctable[0] = calloc(1, DIROBJECTALLOC_MIN_ALLOC);
    if(!allocator -> alloctable[0]) {
      free(allocator);
      return 0;
    }
    allocator -> currtablelimit = DIROBJECTALLOC_MIN_ALLOC;
  }
  return allocator;
}

/* DirObject must be zero initialized */
WFSDIROBJECT * AllocateDirObjectFromWfs(WFSDIROBJECTALLOCATOR * allocator, const wchar_t * name) {
  size_t namesz = wcslen(name);
  size_t objsize = (sizeof(WFSDIROBJECT) / sizeof(wchar_t));
  WFSDIROBJECT * obj;
  if(namesz > SIZE_MAX - objsize - 2)
    return 0;
  objsize += namesz + 2;
  if(objsize > SIZE_MAX / sizeof(wchar_t))
    return 0;
  objsize *= sizeof(wchar_t);
  if(objsize > allocator -> currtablelimit - allocator -> currtablepos) {
    size_t newlimit = allocator -> currtablelimit;
    size_t newidx = allocator -> currtableidx + 1;
    uint8_t * newtable;
    if(newlimit < SIZE_MAX / 2)
      newlimit *= 2;
    if(newlimit < objsize)
      newlimit = objsize;
    newtable = calloc(1, newlimit);
    if(!newtable) {
      newlimit = objsize;
      newtable = calloc(1, newlimit);
      if(!newtable)
        return 0;
    }
    allocator -> alloctable[newidx] = newtable;
    allocator -> currtableidx = newidx;
    allocator -> currtablepos = 0;
    allocator -> currtablelimit = newlimit;
  }
  obj = (WFSDIROBJECT *)(allocator -> alloctable[allocator -> currtableidx] + allocator -> currtablepos);
  allocator -> currtablepos += objsize;
  wcscpy(obj -> name, name);
  obj -> name[namesz] = (wchar_t)'\\';
  return obj; 
}

/* All DirObj names must have a '\' at the end. */
WFSDIROBJECT *CreateDirObject(const wchar_t *name) {
  size_t namesz = wcslen(name);
  if(namesz > SIZE_MAX - 2 - (sizeof(WFSDIROBJECT) / sizeof(wchar_t)))
    return 0;
  WFSDIROBJECT *obj = calloc(sizeof(wchar_t), (sizeof(WFSDIROBJECT) / sizeof(wchar_t)) + namesz + 2);
  if(obj) {
    wcscpy(obj -> name, name);
    obj -> name[namesz] = (wchar_t)'\\';
  }
  return obj;
}

/* 
 * fullbase should be a path returned by GetFullPathNameW,
 * though not doing so is not a fatal error.
 * Remove all backslashes at the end.
 */
WFSFILEPATH *CreateWfsFilePath(const wchar_t * fullbase) {
  WFSFILEPATH *path = malloc(sizeof(path));
  const wchar_t * regprefix = L"\\\\?\\";
  const wchar_t * uncprefix = L"\\\\?\\UNC";
  const wchar_t * prefix = regprefix;
  size_t prefixlen;
  if(!wcsncmp(fullbase, regprefix, wcslen(regprefix))) {
    prefix = L"";
  } else if(!wcsncmp(fullbase, L"\\\\", 2)) {
    fullbase += 1;
    prefix = uncprefix;
  }
  prefixlen = wcslen(prefix);
  if(path) {
    path -> baselen = wcslen(fullbase);
    if(path -> baselen > SIZE_MAX - prefixlen - 1)
      return 0;
    path -> baselen += prefixlen + 1;
    path -> len = path -> baselen;
    path -> alloclen = path -> baselen;
    if(path -> alloclen < PATH_MIN_ALLOC)
      path -> alloclen = PATH_MIN_ALLOC;
    path -> fpath = calloc(sizeof(wchar_t), path -> alloclen);
    if(!path -> fpath) {
      free(path);
      path = 0;
    }
    wcscat(path -> fpath, prefix);
    wcscat(path -> fpath + prefixlen, fullbase);
    if(path -> fpath[path -> len - 2] == '\\') {
      path -> fpath[path -> len-- - 2] = 0;
      path -> baselen -= 1;
    }
  }
  return path;
}

/*
 * Path should be something returned by GetFullPathNameW.
 * Not doing so may make this function fail tremendously.
 */
WFSFILEENUM * CreateFileEnumerator(const wchar_t * path) {
  WFSFILEENUM *fenum = calloc(sizeof(WFSFILEENUM), 1);

  if(fenum) {
    wchar_t * findpath;
    fenum -> maxlevel = INFINITE;
    fenum -> currpath = CreateWfsFilePath(path);
    if(!fenum -> currpath) {
      DestroyFileEnumerator(fenum);
      return 0;
    }
    fenum -> relativedir = fenum -> currpath -> fpath + fenum -> currpath -> baselen;
    fenum -> allocator = CreateDirObjectAllocator();
    if(!fenum -> allocator) {
      DestroyFileEnumerator(fenum);
      return 0;
    }
    if(fenum -> currpath -> fpath[fenum -> currpath -> len - 2] == ':') {
      findpath = fenum -> currpath -> fpath + fenum -> currpath -> len - 3;
    }
    else {
      findpath = fenum -> currpath -> fpath;
    }
    fenum -> dhandle = FindFirstFileW(findpath, &fenum -> result);
    if(fenum -> dhandle == INVALID_HANDLE_VALUE) {
      DestroyFileEnumerator(fenum);
      return 0;
    }
    if(fenum -> result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      fenum -> root = AllocateDirObjectFromWfs(fenum -> allocator, L"");
      if(!fenum -> root) {
        DestroyFileEnumerator(fenum);
        return 0;
      }
      fenum -> next = &fenum -> root;
      fenum -> end = fenum -> root;
      fenum -> result.cFileName[0] = 0;
    }
  }

  return fenum;
}

void DestroyFileEnumerator(WFSFILEENUM *enumerator) {
  if(enumerator) {
    if(enumerator -> currpath)
      DestroyWfsFilePath(enumerator -> currpath);
    if(enumerator -> allocator)
      DestroyDirObjectAllocator(enumerator -> allocator);
    if(enumerator -> dhandle && enumerator -> dhandle != INVALID_HANDLE_VALUE)
      FindClose(enumerator -> dhandle);
    free(enumerator);
  }
}

void DestroyWfsFilePath(WFSFILEPATH * path) {
  free(path -> fpath);
  free(path);
}

void DestroyDirObjectRoot(WFSDIROBJECT *root) {
  WFSDIROBJECT *current = root;
  while(current) {
    WFSDIROBJECT *temp = current -> breadth;
    free(current);
    current = temp;
  }
}

void DestroyDirObjectAllocator(WFSDIROBJECTALLOCATOR * allocator) {
  size_t curridx = allocator -> currtableidx;
  void ** table = allocator -> alloctable;
  for(size_t i = 0; i < curridx; i += 1) {
    free(table[i]);
  }
  free(allocator);
}

wchar_t *PreparePathAppend(WFSFILEPATH *path, SIZE_T nbytes) {
  size_t newlen;
  if(nbytes > SIZE_MAX - path -> len)
    return 0;
  newlen = path -> len + nbytes;
  if(newlen > path -> alloclen) {
    size_t newalloclen;
    wchar_t * newpath;
    if(path -> alloclen > SIZE_MAX / 2 || newlen > path -> alloclen * 2) {
      newalloclen = newlen;
    } else {
      newalloclen = path -> alloclen * 2;
    }
    if(newalloclen > SIZE_MAX / sizeof(wchar_t))
      return 0;
    newpath = realloc(path -> fpath, newalloclen * sizeof(wchar_t));
    if(!newpath) {
      if(newlen < newalloclen) {
        newalloclen = newlen;
        newpath = realloc(path -> fpath, newalloclen * sizeof(wchar_t));
        if(!newpath)
          return 0;
      } else
        return 0;
    }
    path -> fpath = newpath;
    path -> len = newlen;
    path -> alloclen = newalloclen;
  } else {
    path -> len = newlen;
  }
  return path -> fpath + newlen - 1 - nbytes;
}

/*
 *  NOTE: 
 *  If this function fails, path does not store a valid path anymore.
 *  However, you may still try again with any valid arguments, including
 *  the same ones. Note that this will put a '\' at the end.
 */
wchar_t *ConstructPathFromDir(WFSFILEPATH *path, const WFSDIROBJECT *dir, DWORD *pLevel) {
  wchar_t * currdir;
  const WFSDIROBJECT * orig = dir;
  DWORD level = 0;

  path -> len = path -> baselen;
  path -> fpath[path -> baselen - 1] = 0;
  while(dir) {
    const wchar_t * currname = dir -> name;
    size_t dirnamelen = wcslen(currname);
    if(!(currdir = PreparePathAppend(path, dirnamelen)))
      return 0;
    for(size_t i = 0; i < dirnamelen; i += 1) {
      currdir[i] = currname[dirnamelen - 1 - i];
    }
    currdir[dirnamelen] = 0;
    dir = dir -> parent;
    level += 1;
  }
  if(orig) {
    size_t totalsize = path -> len - path -> baselen;
    wchar_t * postbase = path -> fpath + path -> baselen - 1;
    for(size_t i = 0, j = totalsize - 1; i < j; i += 1, j -= 1) {
      wchar_t temp = postbase[i];
      postbase[i] = postbase[j];
      postbase[j] = temp;
    }
  }
  *pLevel = level;
  return path -> fpath;
}

/* Now for the fun part! Enumeration! */

DWORD UnconditionalGetNextFileFromEnumerator(WFSFILEENUM * enumerator) {
  DWORD flags = 0;
  wchar_t * fname = enumerator -> result.cFileName;
  DWORD level;
  if(!enumerator -> done) {
    if(!FindNextFileW(enumerator -> dhandle, &enumerator -> result)) {
      WFSDIROBJECT * diriter = *enumerator -> next;
      for(; diriter; diriter = diriter -> breadth) {
        HANDLE nextfile;
        wchar_t * wildcard;
        const wchar_t * wildcardstr = L"*";
        if(!ConstructPathFromDir(enumerator -> currpath, diriter, &level))
          continue;
        if(!(wildcard = PreparePathAppend(enumerator -> currpath, wcslen(wildcardstr))))
          continue;
        wcscpy(wildcard, wildcardstr);
        if((nextfile = FindFirstFileW(enumerator -> currpath -> fpath, &enumerator -> result)) == INVALID_HANDLE_VALUE)
          continue;
        enumerator -> currpath -> fpath[enumerator -> currpath -> len-- - 2] = 0;
        if(level > enumerator -> currlevel)
          enumerator -> currlevel = level;
        FindClose(enumerator -> dhandle);
        enumerator -> dhandle = nextfile;
        break;
      }
      if(!diriter) {
        enumerator -> done = 1;
        return GETNEXTFILE_FLAG_DONE;
      } else {
        enumerator -> current = diriter;
        enumerator -> next = &diriter -> breadth;
        enumerator -> relativedir = enumerator -> currpath -> fpath + enumerator -> currpath -> baselen;
        flags |= GETNEXTFILE_FLAG_DIRCHANGED;
      }
    }
    if((enumerator -> result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        && wcscmp(L".", fname)
        && wcscmp(L"..", fname)) {
      WFSDIROBJECT * newobj = AllocateDirObjectFromWfs(enumerator -> allocator, fname);
      if(!newobj)
        return GETNEXTFILE_FLAG_DONE;
      newobj -> parent = enumerator -> current;
      enumerator -> end -> breadth = newobj;
      enumerator -> end = newobj;
    }
  }
  return flags;
}

DWORD GetNextFileFromEnumerator(WFSFILEENUM * enumerator) {
  DWORD min = enumerator -> minlevel;
  DWORD max = enumerator -> maxlevel;
  DWORD curr;
  DWORD status;
  wchar_t * fname = enumerator -> result.cFileName;
  do {
    status = UnconditionalGetNextFileFromEnumerator(enumerator);
    curr = enumerator -> currlevel;
  } while(!(status & GETNEXTFILE_FLAG_DONE) && (!(wcscmp(fname, L".") && wcscmp(fname, L"..")) || curr < min));
  if(curr > max)
    return GETNEXTFILE_FLAG_DONE;
  return status;
}
