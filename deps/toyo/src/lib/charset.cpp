#include "winerr.hpp"

#include <clocale>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <stdexcept>

#include "toyo/charset.hpp"

namespace toyo {

namespace charset {

std::wstring a2w(const std::string& str) {
#ifdef _WIN32
  int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
  if (len == 0) {
    throw std::runtime_error(get_win32_last_error_message());
  }
  wchar_t* buf = new wchar_t[len];
  memset(buf, 0, len * sizeof(wchar_t));
  MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, buf, len);
  std::wstring res(buf);
  delete[] buf;
  return res;
#else
  std::string target_locale = "en_US.utf8";
  const char* c_locale = std::setlocale(LC_CTYPE, nullptr);
  std::string locale(c_locale ? c_locale : "");

  if (locale != target_locale) {
    std::setlocale(LC_CTYPE, target_locale.c_str());
  }

  size_t len = std::mbstowcs(nullptr, str.c_str(), 0);
  wchar_t* buf = new wchar_t[len + 1];
  memset(buf, 0, (len + 1) * sizeof(wchar_t));
  std::mbstowcs(buf, str.c_str(), len + 1);
  std::wstring res(buf);
  delete[] buf;

  if (locale != "") {
    std::setlocale(LC_CTYPE, locale.c_str());
  }
  return res;
#endif
}

std::string w2a(const std::wstring& wstr) {
#ifdef _WIN32
  int len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, NULL, NULL);
  if (len == 0) {
    throw std::runtime_error(get_win32_last_error_message());
  }
  char* buf = new char[len];
  memset(buf, 0, len * sizeof(char));
  WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, buf, len, NULL, NULL);
  std::string res(buf);
  delete[] buf;
  return res;
#else
  std::string target_locale = "en_US.utf8";
  const char* c_locale = std::setlocale(LC_CTYPE, nullptr);
  std::string locale(c_locale ? c_locale : "");

  if (locale != target_locale) {
    std::setlocale(LC_CTYPE, target_locale.c_str());
  }

  size_t len = std::wcstombs(nullptr, wstr.c_str(), 0);
  char* buf = new char[len + 1];
  memset(buf, 0, (len + 1) * sizeof(char));
  std::wcstombs(buf, wstr.c_str(), len + 1);
  std::string res(buf);
  delete[] buf;

  if (locale != "") {
    std::setlocale(LC_CTYPE, locale.c_str());
  }
  return res;
#endif
}

static std::string _w2a(const std::wstring& wstr, int code_page) {
#ifdef _WIN32
  int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
  if (len == 0) {
    throw std::runtime_error(get_win32_last_error_message().c_str());
  }
  char* buf = new char[len];
  memset(buf, 0, len * sizeof(char));
  WideCharToMultiByte(code_page, 0, wstr.c_str(), -1, buf, len, nullptr, nullptr);
  std::string res(buf);
  delete[] buf;
  return res;
#else
  return w2a(wstr);
#endif
}

std::string w2acp(const std::wstring& wstr) {
#ifdef _WIN32
  return _w2a(wstr, CP_ACP);
#else
  return w2a(wstr);
#endif
}

std::string a2acp(const std::string& str) {
#ifdef _WIN32
  return w2acp(a2w(str));
#else
  return str;
#endif
}

std::string w2ocp(const std::wstring& wstr) {
#ifdef _WIN32
  return _w2a(wstr, GetConsoleOutputCP());
#else
  return w2a(wstr);
#endif
}

std::string a2ocp(const std::string& str) {
#ifdef _WIN32
  return w2ocp(a2w(str));
#else
  return str;
#endif
}

} // charset

} // toyo
