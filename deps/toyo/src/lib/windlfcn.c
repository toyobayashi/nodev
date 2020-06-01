#ifdef _WIN32

// #include "winerr.hpp"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "windlfcn.h"

/**
 * Win32 error code from last failure.
 */

static DWORD lastError = 0;

static wchar_t* utf82unicode(const char* utf8) {
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
  if (len == 0) {
    return NULL;
  }
  wchar_t* buf = (wchar_t*)malloc(sizeof(wchar_t) * len);
  if (!buf) {
    return NULL;
  }
  memset(buf, 0, sizeof(wchar_t) * len);
  len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, buf, len);
  if (len == 0) {
    free(buf);
    return NULL;
  }

  return buf;
}

static char* unicode2utf8(const wchar_t* unicode) {
  int len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
  if (len == 0) {
    return NULL;
  }
  char* buf = (char*)malloc(sizeof(char) * len);
  if (!buf) {
    return NULL;
  }
  memset(buf, 0, sizeof(char) * len);
  len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, buf, len, NULL, NULL);
  if (len == 0) {
    free(buf);
    return NULL;
  }
  return buf;
}

static char* get_win32_error_message(DWORD code) {
  LPVOID buf;
  if (FormatMessageW(
    FORMAT_MESSAGE_FROM_SYSTEM
    | FORMAT_MESSAGE_IGNORE_INSERTS
    | FORMAT_MESSAGE_ALLOCATE_BUFFER,
    NULL,
    code,
    GetSystemDefaultLangID() /* MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT) */,
    (LPWSTR) &buf,
    0,
    NULL
  )) {
    WCHAR* msg = (wchar_t*)buf;
    msg[wcslen(msg) - 2] = L'\0';
    char* utf8str = unicode2utf8(msg);
    LocalFree(buf);
    return utf8str;
  } else {
    return NULL;
  }
}

/**
 * Open DLL, returning a handle.
 */
void* dlopen(const char* file, int mode) {
  UINT errorMode;
  void* handle;

  UNREFERENCED_PARAMETER(mode);

  if (file == NULL) return (void*) GetModuleHandle(NULL);

  WCHAR* wfile = utf82unicode(file);
  if (!wfile) {
    lastError = GetLastError();
    return NULL;
  }

  errorMode = GetErrorMode();

  /* Have LoadLibrary return NULL on failure; prevent GUI error message. */
  SetErrorMode(errorMode | SEM_FAILCRITICALERRORS);

  handle = (void*) LoadLibraryW(wfile);
  free(wfile);

  if (handle == NULL) lastError = GetLastError();

  SetErrorMode(errorMode);

  return handle;
}

/**
 * Close DLL.
 */
int dlclose(void* handle) {
  int rc = 0;

  if (handle != (void*) GetModuleHandle(NULL))
    rc = !FreeLibrary((HMODULE) handle);

  if (rc)
    lastError = GetLastError();

  return rc;
}

/**
 * Look up symbol exported by DLL.
 */
void* dlsym(void* handle, const char* name) {
  void* address = (void*) GetProcAddress((HMODULE) handle, name);

  if (address == NULL)
    lastError = GetLastError();

  return address;
}

/**
 * Return message describing last error.
 */
char* dlerror() {
  static char errorMessage[260];
  memset(errorMessage, 0, sizeof(errorMessage));

  if (lastError != 0) {
    char* errmsg = get_win32_error_message(lastError);
    if (!errmsg) {
      strcpy(errorMessage, "Win32 error: ");
      char code[10];
      _ultoa(lastError, code, 10);
      strcat(errorMessage, code);
      return errorMessage;
    }
    strcpy(errorMessage, errmsg);
    free(errmsg);
    lastError = 0;
    return errorMessage;
  } else {
    return NULL;
  }
}

#endif
