#ifndef __TOYO_CONSOLE_HPP__
#define __TOYO_CONSOLE_HPP__

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include "Windows.h"

  #define COLOR_RED_BRIGHT (FOREGROUND_RED | FOREGROUND_INTENSITY)
  #define COLOR_YELLOW_BRIGHT (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY)
  #define COLOR_GREEN_BRIGHT (FOREGROUND_GREEN | FOREGROUND_INTENSITY)
#else
#define COLOR_RED_BRIGHT ("\x1b[31;1m")
#define COLOR_YELLOW_BRIGHT ("\x1b[33;1m")
#define COLOR_GREEN_BRIGHT ("\x1b[32;1m")
#define COLOR_RESET ("\x1b[0m")

#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

namespace toyo {

namespace {
  std::string a2ocp(const std::string& str) {
#ifdef _WIN32
    int len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    if (len == 0) {
      return "";
    }
    wchar_t* wstr = new wchar_t[len];
    memset(wstr, 0, len * sizeof(wchar_t));
    len = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, wstr, len);
    if (len == 0) {
      delete[] wstr;
      return "";
    }

    UINT cp = GetConsoleOutputCP();
    len = WideCharToMultiByte(cp, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
      delete[] wstr;
      return "";
    }
    char* buf = new char[len];
    memset(buf, 0, len * sizeof(char));
    len = WideCharToMultiByte(cp, 0, wstr, -1, buf, len, nullptr, nullptr);
    if (len == 0) {
      delete[] wstr;
      delete[] buf;
      return "";
    }
    std::string res(buf);
    delete[] wstr;
    delete[] buf;
    return res;
#else
    return str;
#endif
  }
}

class console {
private:
#ifdef _WIN32
  static WORD _set_console_text_attribute(HANDLE hConsole, WORD wAttr) {
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    if (!GetConsoleScreenBufferInfo(hConsole, &csbiInfo)) return FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
    WORD originalAttr = csbiInfo.wAttributes;
    SetConsoleTextAttribute(hConsole, wAttr);
    return originalAttr;
  }
#endif

  static std::string _format(char);

  static std::string _format(bool);

  static std::string _format(const char*);

  static std::string _format(const std::vector<unsigned char>&);

  static std::string _format(const std::vector<std::string>&);

  static std::string _format(const std::string&);

  static std::string _format(const std::map<std::string, std::string>&);

  template <typename T>
  static std::string _format(const std::vector<T>& arr) {
    size_t len = arr.size();
    std::ostringstream oss;
    if (len == 0) {
      oss << "[]";
      return oss.str();
    }
    oss << "[ ";
    for (size_t i = 0; i < arr.size(); i++) {
      oss << arr[i];
      if (i != arr.size() - 1) {
        oss << ", ";
      }
    }
    oss << " ]";
    return oss.str();
  }

  template <typename T>
  static std::string _format(const T& arg) {
    std::ostringstream oss;
    oss << arg;
    return oss.str();
  }

  static void _logerror(const char*);

  static void _logerror(const std::string&);

  template <typename... Args>
  static void _logerror(const std::string& format, Args... args) {
    std::string f = a2ocp(format) + "\n";
    fprintf(stderr, f.c_str(), args...);
  }

  template <typename T>
  static void _logerror(const T& arg) {
    std::cerr << _format(arg) << std::endl;
  }

public:
  console() = delete;
  console(const console&) = delete;

  static void write(const char*);

  static void write(const std::string&);

  template <typename... Args>
  static void write(const std::string& format, Args... args) {
    printf(a2ocp(format).c_str(), args...);
    fflush(stdout);
  }

  template <typename T>
  static void write(const T& arg) {
    std::cout << _format(arg);
    fflush(stdout);
  }

  static void log(const char* arg);

  static void log(const std::string& arg);

  template <typename... Args>
  static void log(const std::string& format, Args... args) {
    std::string f = a2ocp(format) + "\n";
    printf(f.c_str(), args...);
  }

  template <typename T>
  static void log(const T& arg) {
    std::cout << _format(arg) << std::endl;
  }

  template <typename T>
  static void info(const T& arg) {
#ifdef _WIN32
    HANDLE hconsole = GetStdHandle(STD_OUTPUT_HANDLE);
    WORD original = _set_console_text_attribute(hconsole, COLOR_GREEN_BRIGHT);
    log(arg);
    _set_console_text_attribute(hconsole, original);
#else
    std::cout << COLOR_GREEN_BRIGHT;
    log(arg);
    std::cout << COLOR_RESET;
    fflush(stdout);
#endif
  }

  template <typename T>
  static void warn(const T& arg) {
#ifdef _WIN32
    HANDLE hconsole = GetStdHandle(STD_ERROR_HANDLE);
    WORD original = _set_console_text_attribute(hconsole, COLOR_YELLOW_BRIGHT);
    _logerror(arg);
    _set_console_text_attribute(hconsole, original);
#else
    std::cerr << COLOR_YELLOW_BRIGHT;
    _logerror(arg);
    std::cerr << COLOR_RESET;
    fflush(stderr);
#endif
  }

  template <typename T>
  static void error(const T& arg) {
#ifdef _WIN32
    HANDLE hconsole = GetStdHandle(STD_ERROR_HANDLE);
    WORD original = _set_console_text_attribute(hconsole, COLOR_RED_BRIGHT);
    log(arg);
    _set_console_text_attribute(hconsole, original);
#else
    std::cerr << COLOR_RED_BRIGHT;
    _logerror(arg);
    std::cerr << COLOR_RESET;
    fflush(stderr);
#endif
  }
  static unsigned short get_terminal_width();
  static void clear();
  static void clear_line(short lineNumber = 0);
}; // class console

inline std::string console::_format(char c) {
  char buf[2] = { c, '\0' };
  return std::string(buf);
}

inline std::string console::_format(bool b) {
  return b ? "true" : "false";
}

inline std::string console::_format(const char* cstr) {
  return a2ocp(cstr);
}

inline std::string console::_format(const std::vector<unsigned char>& buf) {
  char _map[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  std::ostringstream oss;
  oss << "<Buffer ";
  for (size_t i = 0; i < buf.size(); i++) {
    oss << _map[buf[i] >> 4];
    oss << _map[buf[i] & 0x0f];
    if (i != buf.size() - 1) {
      oss << " ";
    }
  }
  oss << ">";
  return oss.str();
}

inline std::string console::_format(const std::vector<std::string>& arr) {
  size_t len = arr.size();
  std::ostringstream oss;
  if (len == 0) {
    oss << "[]";
    return oss.str();
  }
  oss << "[ ";
  for (size_t i = 0; i < len; i++) {
    oss << "\"" << a2ocp(arr[i]) << "\"";
    if (i != len - 1) {
      oss << ", ";
    }
  }
  oss << " ]";
  return oss.str();
}

inline std::string console::_format(const std::map<std::string, std::string>& strobj) {
  size_t len = strobj.size();
  std::ostringstream oss;
  if (len == 0) {
    oss << "{}";
    return oss.str();
  }
  oss << "{\n";
  size_t i = 0;
  for (auto& p : strobj) {
    oss << a2ocp(std::string("  \"") + p.first + "\": \"" + p.second + "\"");
    if (i != len - 1) {
      oss << ",\n";
    } else {
      oss << "\n";
    }
    i++;
  }
  oss << "}";
  return oss.str();
}

inline std::string console::_format(const std::string& str) {
  return a2ocp(str);
}

inline void console::_logerror(const char* arg) {
  std::cerr << _format(arg) << std::endl;
}

inline void console::_logerror(const std::string& arg) {
  std::cerr << _format(arg) << std::endl;
}

inline void console::log(const char* arg) {
  std::cout << _format(arg) << std::endl;
}

inline void console::log(const std::string& arg) {
  std::cout << _format(arg) << std::endl;
}

inline void console::write(const char* arg) {
  std::cout << _format(arg);
  fflush(stdout);
}

inline void console::write(const std::string& arg) {
  std::cout << _format(arg);
  fflush(stdout);
}

inline void console::clear() {
#ifdef _WIN32
  HANDLE _consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  COORD coordScreen = { 0, 0 };
  DWORD cCharsWritten;
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  DWORD dwConSize;

  if (!GetConsoleScreenBufferInfo(_consoleHandle, &csbi)) {
    return;
  }

  dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

  if (!FillConsoleOutputCharacter(_consoleHandle,
    (TCHAR) ' ',
    dwConSize,
    coordScreen,
    &cCharsWritten))
  {
    return;
  }

  if (!GetConsoleScreenBufferInfo(_consoleHandle, &csbi)) {
    return;
  }

  if (!FillConsoleOutputAttribute(_consoleHandle,
    csbi.wAttributes,
    dwConSize,
    coordScreen,
    &cCharsWritten))
  {
    return;
  }

  SetConsoleCursorPosition(_consoleHandle, coordScreen);
#else
  std::cout << "\033[2J\033[1;1H";
  fflush(stdout);
#endif
}

inline unsigned short console::get_terminal_width() {
#ifdef _WIN32
  CONSOLE_SCREEN_BUFFER_INFO bInfo;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &bInfo);
  return (unsigned short) bInfo.dwSize.X;
#else
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
#endif
}

inline void console::clear_line(short lineNumber) {
#ifdef _WIN32
  HANDLE _consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(_consoleHandle, &csbi)) return;
  short tmp = csbi.dwCursorPosition.Y - lineNumber;
  COORD targetFirstCellPosition = { 0, tmp < 0 ? 0 : tmp };
  DWORD size = csbi.dwSize.X * (lineNumber + 1);
  DWORD cCharsWritten;

  if (!FillConsoleOutputCharacterW(_consoleHandle, L' ', size, targetFirstCellPosition, &cCharsWritten)) return;
  if (!GetConsoleScreenBufferInfo(_consoleHandle, &csbi)) return;
  if (!FillConsoleOutputAttribute(_consoleHandle, csbi.wAttributes, size, targetFirstCellPosition, &cCharsWritten)) return;
  SetConsoleCursorPosition(_consoleHandle, targetFirstCellPosition);
#else
  unsigned short w = get_terminal_width() - 1;
  char* b = new char[w + 1];
  memset(b, (int)' ', w);
  *(b + w) = '\0';
  for (short i = 0; i < lineNumber; i++) {
    std::cout << "\r" << b << "\r\x1b[1A";
  }
  std::cout << "\r" << b << "\r";
  delete[] b;
  fflush(stdout);
#endif
}

} // namespace toyo

#endif
