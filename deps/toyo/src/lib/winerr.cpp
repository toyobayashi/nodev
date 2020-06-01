#ifdef _WIN32

#include <stdexcept>
#include "winerr.hpp"
#include "toyo/charset.hpp"

std::string get_win32_error_message(DWORD code) {
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
    std::wstring msg = (wchar_t*)buf;
    LocalFree(buf);
    size_t pos = msg.find_last_of(L"\r\n");
    msg = msg.substr(0, pos - 1);
    std::string utf8str = toyo::charset::w2a(msg);

    return utf8str;
  } else {
    char buf[10] = { 0 };
    _itoa(GetLastError(), buf, 10);
    throw std::runtime_error((std::string("Cannot format message. Win32 error code: ") + buf).c_str());
  }
}

std::string get_win32_last_error_message() {
  return get_win32_error_message(GetLastError());
}

#endif
