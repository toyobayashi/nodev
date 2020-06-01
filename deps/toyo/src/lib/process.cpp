#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include "windlfcn.h"
#else
#include <unistd.h>
#include <dlfcn.h>
#include <dirent.h>
#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char** environ;
#endif
#endif

#include <cstdlib>

#include "toyo/process.hpp"
#include "toyo/charset.hpp"
#include "string.hpp"

namespace toyo {

namespace process {

std::string cwd() {
#ifdef _WIN32
  wchar_t* buf;
  if ((buf = _wgetcwd(nullptr, 0)) == nullptr) {
    return "";
  }
  std::wstring res = buf;
  free(buf);
  return toyo::charset::w2a(res);
#else
  char* buf;
  if ((buf = getcwd(nullptr, 0)) == nullptr) {
    return "";
  }
  std::string res = buf;
  free(buf);
  return res;
#endif
}

int pid() {
#ifdef _WIN32
  return _getpid();
#else
  return getpid();
#endif
}

std::string platform() {
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  return "win32";
#elif defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
  #include <TargetConditionals.h>
  #if defined(TARGET_OS_MAC) && TARGET_OS_MAC
    return "darwin";
    // #define I_OS_DARWIN
    // #ifdef __LP64__
    //   #define I_OS_DARWIN64
    // #else
    //   #define I_OS_DARWIN32
    // #endif
  #else
    return "unknown";
  #endif
#elif defined(__ANDROID__) || defined(ANDROID)
  return "android";
#elif defined(__linux__) || defined(__linux)
  return "linux";
#else
  return "unknown";
#endif
}

void* dlopen(const std::string& file, int mode) {
  return ::dlopen(file.c_str(), mode);
}
int dlclose(void* handle) {
  return ::dlclose(handle);
}
void* dlsym(void* handle, const std::string& name) {
  return ::dlsym(handle, name.c_str());
}
std::string dlerror() {
  return ::dlerror();
}

std::map<std::string, std::string> env() {
  static std::map<std::string, std::string> _env;
  if (_env.size() == 0) {
#ifdef _WIN32
    wchar_t* environment = GetEnvironmentStringsW();
    if (environment == nullptr) return _env;
    wchar_t* p = environment;
    while (*p) {
      if (*p == L'=') {
        p += wcslen(p) + 1;
        continue;
      }
      std::wstring e = p;
      std::vector<std::wstring> keyvalue = toyo::string::wsplit(e, L"=");
      _env[toyo::charset::w2a(keyvalue[0])] = toyo::charset::w2a(keyvalue[1]);

      p += wcslen(p) + 1;
    }
    FreeEnvironmentStringsW(environment);
#else
    int env_size = 0;
    while (environ[env_size]) {
      std::wstring e = toyo::charset::a2w(environ[env_size]);
      std::vector<std::wstring> keyvalue = toyo::string::wsplit(e, L"=");
      _env[toyo::charset::w2a(keyvalue[0])] = toyo::charset::w2a(keyvalue[1]);
      env_size++;
    }
#endif
  }
  return _env;
}

} // process

} // toyo
