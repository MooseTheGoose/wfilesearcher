#include "enumfiles.h"
#include "wfilesearcher.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <stdint.h>
#include <stdio.h>

#define PATH_MIN_ALLOC 2048

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

/* All DirObj names must have a '\' at the end. */
WFSDIROBJECT *CreateDirObject(const wchar_t *name) {
  size_t namesz = wcslen(name);
  if(namesz == SIZE_MAX)
    return 0;
  namesz += 1;
  WFSDIROBJECT *obj = calloc(sizeof(wchar_t), (sizeof(WFSDIROBJECT) / sizeof(wchar_t)) + namesz + 1);
  if(obj) {
    wcscpy(obj -> name, name);
    obj -> name[namesz - 1] = (wchar_t)'\\';
  }
  return obj;
}

/* 
 * fullbase should be a path returned by GetFullPathNameW,
 * though not doing so is not a fatal error.
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
  }
  return path;
}

WFSFILEENUM * CreateFileEnumerator(const wchar_t * path) {
  WFSFILEENUM *fenum = calloc(sizeof(WFSFILEENUM), 1);

  if(fenum) {
    wchar_t * wcfullpath;
    if(!(wcfullpath = AllocFullPathName(path))) {
      free(fenum);
      return 0;
    }
    fenum -> currpath = CreateWfsFilePath(wcfullpath);
    free(wcfullpath);
    if(!fenum -> currpath) {
      free(fenum);
      return 0;
    }
    fenum -> dhandle = FindFirstFileW(fenum -> currpath -> fpath, &fenum -> result);
    if(fenum -> dhandle == INVALID_HANDLE_VALUE) {
      DestroyWfsFilePath(fenum -> currpath);
      free(fenum);
      return 0;
    }
    if(fenum -> result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      fenum -> root = CreateDirObject(L"");
      if(!fenum -> root) {
        FindClose(fenum -> dhandle);
        DestroyWfsFilePath(fenum -> currpath);
        free(fenum);
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
  DestroyWfsFilePath(enumerator -> currpath);
  DestroyDirObjectRoot(enumerator -> root);
  FindClose(enumerator -> dhandle);
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
wchar_t *ConstructPathFromDir(WFSFILEPATH *path, const WFSDIROBJECT *dir) {
  wchar_t * currdir;
  const WFSDIROBJECT * orig = dir;

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
  return path -> fpath;
}

/* Now for the fun part! Enumeration! */

wchar_t * NextFileName(WFSFILEENUM * enumerator, DWORD * attributes, const wchar_t ** dirstr) {
  wchar_t * fname = 0;
  if(!enumerator -> done) {
    if(!FindNextFileW(enumerator -> dhandle, &enumerator -> result)) {
      WFSDIROBJECT * diriter = *enumerator -> next;
      for(; diriter; diriter = diriter -> breadth) {
        HANDLE nextfile;
        wchar_t * wildcard;
        const wchar_t * wildcardstr = L"*";
        if(!ConstructPathFromDir(enumerator -> currpath, diriter))
          continue;
        if(!(wildcard = PreparePathAppend(enumerator -> currpath, wcslen(wildcardstr))))
          continue;
        wcscpy(wildcard, wildcardstr);
        if(!(nextfile = FindFirstFileW(enumerator -> currpath -> fpath, &enumerator -> result)))
          continue;
        enumerator -> dhandle = nextfile;
        break;
      }
      if(!diriter) {
        enumerator -> done = 1;
        return 0;
      } else {
        enumerator -> current = diriter;
        enumerator -> next = &diriter -> breadth;
      }
    }
    fname = enumerator -> result.cFileName;
    if((enumerator -> result.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        && wcscmp(L".", fname)
        && wcscmp(L"..", fname)) {
      WFSDIROBJECT * newobj = CreateDirObject(fname);
      if(!newobj)
        return 0;
      newobj -> parent = enumerator -> current;
      enumerator -> end -> breadth = newobj;
      enumerator -> end = newobj;
    }
    if(dirstr)
      *dirstr = enumerator -> currpath -> fpath + enumerator -> currpath -> baselen;
    if(attributes)
      *attributes = enumerator -> result.dwFileAttributes;
  }
  return fname;
}
