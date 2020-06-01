#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <userenv.h>
#include "winerr.hpp"
#endif

#include <vector>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <stdexcept>

#ifndef _WIN32

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#if defined(__ANDROID_API__) && __ANDROID_API__ < 21
# include <dlfcn.h>  /* for dlsym */
#endif

#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "toyo/path.hpp"
#include "toyo/charset.hpp"
#include "toyo/process.hpp"
#include <cerrno>
#include "cerror.hpp"
#include "string.hpp"

namespace toyo {

namespace path {

// Alphabet chars.
const unsigned short CHAR_UPPERCASE_A = 65; /* A */
const unsigned short CHAR_LOWERCASE_A = 97; /* a */
const unsigned short CHAR_UPPERCASE_Z = 90; /* Z */
const unsigned short CHAR_LOWERCASE_Z = 122; /* z */

// Non-alphabetic chars.
const unsigned short CHAR_DOT = 46; /* . */
const unsigned short CHAR_FORWARD_SLASH = 47; /* / */
const unsigned short CHAR_BACKWARD_SLASH = 92; /* \ */
const unsigned short CHAR_VERTICAL_LINE = 124; /* | */
const unsigned short CHAR_COLON = 58; /* : */
const unsigned short CHAR_QUESTION_MARK = 63; /* ? */
const unsigned short CHAR_UNDERSCORE = 95; /* _ */
const unsigned short CHAR_LINE_FEED = 10; /* \n */
const unsigned short CHAR_CARRIAGE_RETURN = 13; /* \r */
const unsigned short CHAR_TAB = 9; /* \t */
const unsigned short CHAR_FORM_FEED = 12; /* \f */
const unsigned short CHAR_EXCLAMATION_MARK = 33; /* ! */
const unsigned short CHAR_HASH = 35; /* # */
const unsigned short CHAR_SPACE = 32; /*   */
const unsigned short CHAR_NO_BREAK_SPACE = 160; /* \u00A0 */
const unsigned short CHAR_ZERO_WIDTH_NOBREAK_SPACE = 65279; /* \uFEFF */
const unsigned short CHAR_LEFT_SQUARE_BRACKET = 91; /* [ */
const unsigned short CHAR_RIGHT_SQUARE_BRACKET = 93; /* ] */
const unsigned short CHAR_LEFT_ANGLE_BRACKET = 60; /* < */
const unsigned short CHAR_RIGHT_ANGLE_BRACKET = 62; /* > */
const unsigned short CHAR_LEFT_CURLY_BRACKET = 123; /* { */
const unsigned short CHAR_RIGHT_CURLY_BRACKET = 125; /* } */
const unsigned short CHAR_HYPHEN_MINUS = 45; /* - */
const unsigned short CHAR_PLUS = 43; /* + */
const unsigned short CHAR_DOUBLE_QUOTE = 34; /* " */
const unsigned short CHAR_SINGLE_QUOTE = 39; /* ' */
const unsigned short CHAR_PERCENT = 37; /* % */
const unsigned short CHAR_SEMICOLON = 59; /* ; */
const unsigned short CHAR_CIRCUMFLEX_ACCENT = 94; /* ^ */
const unsigned short CHAR_GRAVE_ACCENT = 96; /* ` */
const unsigned short CHAR_AT = 64; /* @ */
const unsigned short CHAR_AMPERSAND = 38; /* & */
const unsigned short CHAR_EQUAL = 61; /* = */

// Digits
const unsigned short CHAR_0 = 48; /* 0 */
const unsigned short CHAR_9 = 57; /* 9 */

static bool _isPathSeparator(unsigned short code) {
  return code == CHAR_FORWARD_SLASH || code == CHAR_BACKWARD_SLASH;
}
static bool _isPosixPathSeparator(unsigned short code) {
  return code == CHAR_FORWARD_SLASH;
}
static bool _isWindowsDeviceRoot(unsigned short code) {
  return (code >= CHAR_UPPERCASE_A && code <= CHAR_UPPERCASE_Z) || (code >= CHAR_LOWERCASE_A && code <= CHAR_LOWERCASE_Z);
}

static std::wstring _normalizeString(const std::wstring& path, bool allowAboveRoot, const std::wstring& separator, bool (*isPathSeparator)(unsigned short)) {
  std::wstring res = L"";
  int lastSegmentLength = 0;
  int lastSlash = -1;
  int dots = 0;
  unsigned short code = 0;
  for (int i = 0; i <= (int)path.length(); ++i) {
    if (i < (int)path.length())
      code = path[i];
    else if (isPathSeparator(code))
      break;
    else
      code = CHAR_FORWARD_SLASH;

    if (isPathSeparator(code)) {
      if (lastSlash == i - 1 || dots == 1) {
        // NOOP
      } else if (lastSlash != i - 1 && dots == 2) {
        if (res.length() < 2 || lastSegmentLength != 2 ||
          res[res.length() - 1] != CHAR_DOT ||
          res[res.length() - 2] != CHAR_DOT) {
          if (res.length() > 2) {
            const int lastSlashIndex = toyo::string::wlast_index_of(res, separator);
            if (lastSlashIndex == -1) {
              res = L"";
              lastSegmentLength = 0;
            } else {
              res = toyo::string::wslice(res, 0, lastSlashIndex);
              lastSegmentLength = (int)res.length() - 1 - toyo::string::wlast_index_of(res, separator);
            }
            lastSlash = i;
            dots = 0;
            continue;
          } else if (res.length() == 2 || res.length() == 1) {
            res = L"";
            lastSegmentLength = 0;
            lastSlash = i;
            dots = 0;
            continue;
          }
        }
        if (allowAboveRoot) {
          if (res.length() > 0)
            res += (separator + L"..");
          else
            res = L"..";
          lastSegmentLength = 2;
        }
      } else {
        if (res.length() > 0)
          res += separator + toyo::string::wslice(path, lastSlash + 1, i);
        else
          res = toyo::string::wslice(path, lastSlash + 1, i);
        lastSegmentLength = i - lastSlash - 1;
      }
      lastSlash = i;
      dots = 0;
    } else if (code == CHAR_DOT && dots != -1) {
      ++dots;
    } else {
      dots = -1;
    }
  }
  return res;
}

namespace win32 {
  const std::string EOL = "\r\n";

  std::string normalize(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    const int len = (int)path.length();
    if (len == 0)
      return ".";
    int rootEnd = 0;
    std::wstring* device = nullptr;
    bool isAbsolute = false;
    const unsigned short code = path[0];

    if (len > 1) {
      if (_isPathSeparator(code)) {
        isAbsolute = true;

        if (_isPathSeparator(path[1])) {
          int j = 2;
          int last = j;
          for (; j < len; ++j) {
            if (_isPathSeparator(path[j]))
              break;
          }
          if (j < len && j != last) {
            std::wstring firstPart = toyo::string::wslice(path, last, j);
            last = j;
            for (; j < len; ++j) {
              if (!_isPathSeparator(path[j]))
                break;
            }
            if (j < len && j != last) {
              last = j;
              for (; j < len; ++j) {
                if (_isPathSeparator(path[j]))
                  break;
              }
              if (j == len) {

                return toyo::charset::w2a(std::wstring(L"\\\\") + firstPart + L'\\' + toyo::string::wslice(path, last) + L'\\');
              } else if (j != last) {
                std::wstring tmp = std::wstring(L"\\\\") + firstPart + L'\\' + toyo::string::wslice(path, last, j);
                device = new std::wstring(tmp);
                rootEnd = j;
              }
            }
          }
        } else {
          rootEnd = 1;
        }
      } else if (_isWindowsDeviceRoot(code)) {

        if (path[1] == CHAR_COLON) {
          std::wstring tmp = toyo::string::wslice(path, 0, 2);
          device = new std::wstring(tmp);
          rootEnd = 2;
          if (len > 2) {
            if (_isPathSeparator(path[2])) {
              isAbsolute = true;
              rootEnd = 3;
            }
          }
        }
      }
    } else if (_isPathSeparator(code)) {
      return "\\";
    }

    std::wstring tail;
    if (rootEnd < len) {
      tail = _normalizeString(toyo::string::wslice(path, rootEnd), !isAbsolute, L"\\", _isPathSeparator);
    } else {
      tail = L"";
    }
    if (tail.length() == 0 && !isAbsolute)
      tail = L'.';
    if (tail.length() > 0 && _isPathSeparator(path[len - 1]))
      tail += L'\\';
    if (device == nullptr) {
      if (isAbsolute) {
        if (tail.length() > 0)
          return toyo::charset::w2a(std::wstring(L"\\") + tail);
        else
          return "\\";
      } else if (tail.length() > 0) {
        return toyo::charset::w2a(tail);
      } else {
        return "";
      }
    } else if (isAbsolute) {
      if (tail.length() > 0) {
        std::wstring _device = *device;
        delete device;
        return toyo::charset::w2a(_device + L'\\' + tail);
      } else {
        std::wstring _device = *device;
        delete device;
        return toyo::charset::w2a(_device + L'\\');
      }
    } else if (tail.length() > 0) {
      std::wstring _device = *device;
      delete device;
      return toyo::charset::w2a(_device + tail);
    } else {
      std::wstring _device = *device;
      delete device;
      return toyo::charset::w2a(_device);
    }
  }

  std::string resolve(const std::string& arg, const std::string& arg1) {
    std::vector<std::wstring> arguments = { toyo::charset::a2w(arg), toyo::charset::a2w(arg1) };

    std::wstring resolvedDevice = L"";
    std::wstring resolvedTail = L"";
    bool resolvedAbsolute = false;

    for (int i = (int)arguments.size() - 1; i >= -1; i--) {
      std::wstring path = L"";
      if (i >= 0) {
        path = arguments[i];
      } else if (resolvedDevice == L"") {
        path = toyo::charset::a2w(toyo::process::cwd());
      } else {
#ifdef _WIN32
        wchar_t env[MAX_PATH + 1];
        if (0 == GetEnvironmentVariableW((std::wstring(L"=") + resolvedDevice).c_str(), env, MAX_PATH + 1)) {
          path = toyo::charset::a2w(process::cwd());
        } else {
          path = env;
        }
#else
        path = toyo::charset::a2w(process::cwd());
#endif
        if (path == L"" ||
          toyo::string::wto_lower_case(toyo::string::wslice(path, 0, 3)) !=
          toyo::string::wto_lower_case(resolvedDevice) + L'\\') {
          path = resolvedDevice + L'\\';
        }
      }

      if (path.length() == 0) {
        continue;
      }

      int len = (int)path.length();
      int rootEnd = 0;
      std::wstring device = L"";
      bool isAbsolute = false;
      const unsigned short code = path[0];

      if (len > 1) {
        if (_isPathSeparator(code)) {
          isAbsolute = true;

          if (_isPathSeparator(path[1])) {
            int j = 2;
            int last = j;
            for (; j < len; ++j) {
              if (_isPathSeparator(path[j]))
                break;
            }
            if (j < len && j != last) {
              const std::wstring firstPart = toyo::string::wslice(path, last, j);
              last = j;
              for (; j < len; ++j) {
                if (!_isPathSeparator(path[j]))
                  break;
              }
              if (j < len && j != last) {
                last = j;
                for (; j < len; ++j) {
                  if (_isPathSeparator(path[j]))
                    break;
                }
                if (j == len) {
                  device = std::wstring(L"\\\\") + firstPart + L'\\' + toyo::string::wslice(path, last);
                  rootEnd = j;
                } else if (j != last) {
                  device = std::wstring(L"\\\\") + firstPart + L'\\' + toyo::string::wslice(path, last, j);
                  rootEnd = j;
                }
              }
            }
          } else {
            rootEnd = 1;
          }
        } else if (_isWindowsDeviceRoot(code)) {

          if (path[1] == CHAR_COLON) {
            device = toyo::string::wslice(path, 0, 2);
            rootEnd = 2;
            if (len > 2) {
              if (_isPathSeparator(path[2])) {
                isAbsolute = true;
                rootEnd = 3;
              }
            }
          }
        }
      } else if (_isPathSeparator(code)) {
        rootEnd = 1;
        isAbsolute = true;
      }

      if (device.length() > 0 &&
        resolvedDevice.length() > 0 &&
        toyo::string::wto_lower_case(device) != toyo::string::wto_lower_case(resolvedDevice)) {
        continue;
      }

      if (resolvedDevice.length() == 0 && device.length() > 0) {
        resolvedDevice = device;
      }
      if (!resolvedAbsolute) {
        resolvedTail = toyo::string::wslice(path, rootEnd) + L'\\' + resolvedTail;
        resolvedAbsolute = isAbsolute;
      }

      if (resolvedDevice.length() > 0 && resolvedAbsolute) {
        break;
      }
    }
    resolvedTail = _normalizeString(resolvedTail, !resolvedAbsolute, L"\\",
      _isPathSeparator);

    std::wstring res = resolvedDevice + (resolvedAbsolute ? std::wstring(L"\\") : std::wstring(L"")) + resolvedTail;

    return toyo::charset::w2a(res == L"" ? L"." : res);
  }

  std::string join(const std::string& arg1, const std::string& arg2) {
    const std::wstring warg1 = toyo::charset::a2w(arg1);
    const std::wstring warg2 = toyo::charset::a2w(arg2);

    std::wstring joined = L"";
    std::wstring firstPart = L"";

    if (warg1.length() > 0) {
      if (joined == L"")
        joined = firstPart = warg1;
      else
        joined += (std::wstring(L"\\") + warg1);
    }

    if (warg2.length() > 0) {
      if (joined == L"")
        joined = firstPart = warg2;
      else
        joined += (std::wstring(L"\\") + warg2);
    }

    if (joined == L"")
      return ".";

    bool needsReplace = true;
    int slashCount = 0;
    if (_isPathSeparator(firstPart[0])) {
      ++slashCount;
      const size_t firstLen = firstPart.length();
      if (firstLen > 1) {
        if (_isPathSeparator(firstPart[1])) {
          ++slashCount;
          if (firstLen > 2) {
            if (_isPathSeparator(firstPart[2]))
              ++slashCount;
            else {
              needsReplace = false;
            }
          }
        }
      }
    }
    if (needsReplace) {
      for (; slashCount < (int)joined.length(); ++slashCount) {
        if (!_isPathSeparator(joined[slashCount]))
          break;
      }

      if (slashCount >= 2)
        joined = (std::wstring(L"\\") + toyo::string::wslice(joined, slashCount));
    }

    return normalize(toyo::charset::w2a(joined));
  }

  bool is_absolute(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    const size_t len = path.length();
    if (len == 0)
      return false;

    const unsigned short code = path[0];
    if (_isPathSeparator(code)) {
      return true;
    } else if (_isWindowsDeviceRoot(code)) {

      if (len > 2 && path[1] == CHAR_COLON) {
        if (_isPathSeparator(path[2]))
          return true;
      }
    }
    return false;
  }

  std::string dirname(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    const int len = (int)path.length();
    if (len == 0)
      return ".";
    int rootEnd = -1;
    int end = -1;
    bool matchedSlash = true;
    int offset = 0;
    unsigned short code = path[0];

    if (len > 1) {
      if (_isPathSeparator(code)) {

        rootEnd = offset = 1;

        if (_isPathSeparator(path[1])) {
          int j = 2;
          int last = j;
          for (; j < len; ++j) {
            if (_isPathSeparator(path[j]))
              break;
          }
          if (j < len && j != last) {
            last = j;
            for (; j < len; ++j) {
              if (!_isPathSeparator(path[j]))
                break;
            }
            if (j < len && j != last) {
              last = j;
              for (; j < len; ++j) {
                if (_isPathSeparator(path[j]))
                  break;
              }
              if (j == len) {
                return toyo::charset::w2a(path);
              }
              if (j != last) {
                rootEnd = offset = j + 1;
              }
            }
          }
        }
      } else if (_isWindowsDeviceRoot(code)) {
        if (path[1] == CHAR_COLON) {
          rootEnd = offset = 2;
          if (len > 2) {
            if (_isPathSeparator(path[2]))
              rootEnd = offset = 3;
          }
        }
      }
    } else if (_isPathSeparator(code)) {
      return toyo::charset::w2a(path);
    }

    for (int i = len - 1; i >= offset; --i) {
      if (_isPathSeparator(path[i])) {
        if (!matchedSlash) {
          end = i;
          break;
        }
      } else {
        matchedSlash = false;
      }
    }

    if (end == -1) {
      if (rootEnd == -1)
        return ".";
      else
        end = rootEnd;
    }
    return toyo::charset::w2a(toyo::string::wslice(path, 0, end));
  }

  std::string to_namespaced_path(const std::string& p) {
    if (p == "") {
      return "";
    }

    const std::wstring resolvedPath = toyo::charset::a2w(win32::resolve(p));

    if (resolvedPath.length() >= 3) {
      if (resolvedPath[0] == CHAR_BACKWARD_SLASH) {

        if (resolvedPath[1] == CHAR_BACKWARD_SLASH) {
          const int code = resolvedPath[2];
          if (code != CHAR_QUESTION_MARK && code != CHAR_DOT) {
            return toyo::charset::w2a(std::wstring(L"\\\\?\\UNC\\") + toyo::string::wslice(resolvedPath, 2));
          }
        }
      } else if (_isWindowsDeviceRoot(resolvedPath[0])) {

        if (resolvedPath[1] == CHAR_COLON &&
          resolvedPath[2] == CHAR_BACKWARD_SLASH) {
          return toyo::charset::w2a(std::wstring(L"\\\\?\\") + resolvedPath);
        }
      }
    }

    return p;
  }

  std::string basename(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    int start = 0;
    int end = -1;
    bool matchedSlash = true;
    int i;

    if (path.length() >= 2) {
      const unsigned short drive = path[0];
      if (_isWindowsDeviceRoot(drive)) {
        if (path[1] == CHAR_COLON)
          start = 2;
      }
    }

    for (i = (int)path.length() - 1; i >= start; --i) {
      if (_isPathSeparator(path[i])) {
        if (!matchedSlash) {
          start = i + 1;
          break;
        }
      } else if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
    }

    if (end == -1)
      return "";
    return toyo::charset::w2a(toyo::string::wslice(path, start, end));
  }

  std::string basename(const std::string& p, const std::string& e) {
    std::wstring path = toyo::charset::a2w(p);
    std::wstring ext = toyo::charset::a2w(e);

    int start = 0;
    int end = -1;
    bool matchedSlash = true;
    int i;

    if (path.length() >= 2) {
      const unsigned short drive = path[0];
      if (_isWindowsDeviceRoot(drive)) {
        if (path[1] == CHAR_COLON)
          start = 2;
      }
    }

    if (ext.length() > 0 && ext.length() <= path.length()) {
      if (ext.length() == path.length() && ext == path)
        return "";
      int extIdx = (int)ext.length() - 1;
      int firstNonSlashEnd = -1;
      for (i = (int)path.length() - 1; i >= start; --i) {
        const int code = path[i];
        if (_isPathSeparator(code)) {
          if (!matchedSlash) {
            start = i + 1;
            break;
          }
        } else {
          if (firstNonSlashEnd == -1) {
            matchedSlash = false;
            firstNonSlashEnd = i + 1;
          }
          if (extIdx >= 0) {
            if (code == ext[extIdx]) {
              if (--extIdx == -1) {
                end = i;
              }
            } else {
              extIdx = -1;
              end = firstNonSlashEnd;
            }
          }
        }
      }

      if (start == end)
        end = firstNonSlashEnd;
      else if (end == -1)
        end = (int)path.length();
      return toyo::charset::w2a(toyo::string::wslice(path, start, end));
    }

    return win32::basename(p);
  }

  std::string extname(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    int start = 0;
    int startDot = -1;
    int startPart = 0;
    int end = -1;
    bool matchedSlash = true;
    int preDotState = 0;

    if (path.length() >= 2 &&
      path[1] == CHAR_COLON &&
      _isWindowsDeviceRoot(path[0])) {
      start = startPart = 2;
    }

    for (int i = (int)path.length() - 1; i >= start; --i) {
      const unsigned short code = path[i];
      if (_isPathSeparator(code)) {
        if (!matchedSlash) {
          startPart = i + 1;
          break;
        }
        continue;
      }
      if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
      if (code == CHAR_DOT) {
        if (startDot == -1)
          startDot = i;
        else if (preDotState != 1)
          preDotState = 1;
      } else if (startDot != -1) {
        preDotState = -1;
      }
    }

    if (startDot == -1 ||
      end == -1 ||
      preDotState == 0 ||
      (preDotState == 1 &&
        startDot == end - 1 &&
        startDot == startPart + 1)) {
      return "";
    }
    return toyo::charset::w2a(toyo::string::wslice(path, startDot, end));
  }

  std::string relative(const std::string& f, const std::string& t) {
    std::wstring from = toyo::charset::a2w(f);
    std::wstring to = toyo::charset::a2w(t);

    if (from == to)
      return "";

    std::wstring fromOrig = toyo::charset::a2w(win32::resolve(f));
    std::wstring toOrig = toyo::charset::a2w(win32::resolve(t));

    if (fromOrig == toOrig)
      return "";

    from = toyo::string::wto_lower_case(fromOrig);
    to = toyo::string::wto_lower_case(toOrig);

    if (from == to)
      return "";

    int fromStart = 0;
    for (; fromStart < (int)from.length(); ++fromStart) {
      if (from[fromStart] != CHAR_BACKWARD_SLASH)
        break;
    }
    int fromEnd = (int)from.length();
    for (; fromEnd - 1 > fromStart; --fromEnd) {
      if (from[fromEnd - 1] != CHAR_BACKWARD_SLASH)
        break;
    }
    int fromLen = (fromEnd - fromStart);

    int toStart = 0;
    for (; toStart < (int)to.length(); ++toStart) {
      if (to[toStart] != CHAR_BACKWARD_SLASH)
        break;
    }
    int toEnd = (int)to.length();
    for (; toEnd - 1 > toStart; --toEnd) {
      if (to[toEnd - 1] != CHAR_BACKWARD_SLASH)
        break;
    }
    int toLen = (toEnd - toStart);

    int length = (fromLen < toLen ? fromLen : toLen);
    int lastCommonSep = -1;
    int i = 0;
    for (; i <= length; ++i) {
      if (i == length) {
        if (toLen > length) {
          if (to[toStart + i] == CHAR_BACKWARD_SLASH) {
            return toyo::charset::w2a(toyo::string::wslice(toOrig, toStart + i + 1));
          } else if (i == 2) {
            return toyo::charset::w2a(toyo::string::wslice(toOrig, toStart + i));
          }
        }
        if (fromLen > length) {
          if (from[fromStart + i] == CHAR_BACKWARD_SLASH) {
            lastCommonSep = i;
          } else if (i == 2) {
            lastCommonSep = 3;
          }
        }
        break;
      }
      int fromCode = from[fromStart + i];
      int toCode = to[toStart + i];
      if (fromCode != toCode)
        break;
      else if (fromCode == CHAR_BACKWARD_SLASH)
        lastCommonSep = i;
    }

    if (i != length && lastCommonSep == -1) {
      return toyo::charset::w2a(toOrig);
    }

    std::wstring out = L"";
    if (lastCommonSep == -1)
      lastCommonSep = 0;

    for (i = fromStart + lastCommonSep + 1; i <= fromEnd; ++i) {
      if (i == fromEnd || from[i] == CHAR_BACKWARD_SLASH) {
        if (out.length() == 0)
          out += L"..";
        else
          out += L"\\..";
      }
    }

    if (out.length() > 0)
      return toyo::charset::w2a(out + toyo::string::wslice(toOrig, toStart + lastCommonSep, toEnd));
    else {
      toStart += lastCommonSep;
      if (toOrig[toStart] == CHAR_BACKWARD_SLASH)
        ++toStart;
      return toyo::charset::w2a(toyo::string::wslice(toOrig, toStart, toEnd));
    }
  }
} // win32

namespace posix {
  const std::string EOL = "n";

  std::string normalize(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);

    if (path.length() == 0)
      return ".";

    const bool isAbsolute = (path[0] == CHAR_FORWARD_SLASH);
    const bool trailingSeparator =
      (path[path.length() - 1] == CHAR_FORWARD_SLASH);

    std::wstring newpath = _normalizeString(path, !isAbsolute, L"/", _isPosixPathSeparator);

    if (newpath.length() == 0 && !isAbsolute)
      newpath = L'.';
    if (newpath.length() > 0 && trailingSeparator)
      newpath += L'/';

    if (isAbsolute)
      return toyo::charset::w2a(std::wstring(L"/") + newpath);
    return toyo::charset::w2a(newpath);
  }

  std::string resolve(const std::string& arg, const std::string& arg1) {
    std::vector<std::wstring> arguments = { toyo::charset::a2w(arg), toyo::charset::a2w(arg1) };

    std::wstring resolvedPath = L"";
    bool resolvedAbsolute = false;

    for (int i = (int)arguments.size() - 1; i >= -1 && !resolvedAbsolute; i--) {
      std::wstring path;
      if (i >= 0)
        path = arguments[i];
      else {
        path = toyo::charset::a2w(toyo::process::cwd());
      }

      if (path.length() == 0) {
        continue;
      }

      resolvedPath = path + L'/' + resolvedPath;
      resolvedAbsolute = (path[0] == CHAR_FORWARD_SLASH);
    }

    resolvedPath = _normalizeString(resolvedPath, !resolvedAbsolute, L"/",
      _isPosixPathSeparator);

    if (resolvedAbsolute) {
      if (resolvedPath.length() > 0)
        return toyo::charset::w2a(std::wstring(L"/") + resolvedPath);
      else
        return toyo::charset::w2a(std::wstring(L"/"));
    } else if (resolvedPath.length() > 0) {
      return toyo::charset::w2a(resolvedPath);
    } else {
      return ".";
    }
  }

  std::string join(const std::string& arg1, const std::string& arg2) {
    const std::wstring warg1 = toyo::charset::a2w(arg1);
    const std::wstring warg2 = toyo::charset::a2w(arg2);

    if (warg1 == L"" && warg2 == L"")
      return ".";
    std::wstring joined = L"";

    if (warg1.length() > 0) {
      if (joined == L"")
        joined = warg1;
      else
        joined += (std::wstring(L"/") + warg1);
    }

    if (warg2.length() > 0) {
      if (joined == L"")
        joined = warg2;
      else
        joined += (std::wstring(L"/") + warg2);
    }

    if (joined == L"")
      return ".";
    return normalize(toyo::charset::w2a(joined));
  }

  bool is_absolute(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    return path.length() > 0 && path[0] == CHAR_FORWARD_SLASH;
  }

  std::string dirname(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    if (path.length() == 0)
      return ".";
    const bool hasRoot = (path[0] == CHAR_FORWARD_SLASH);
    int end = -1;
    bool matchedSlash = true;
    for (int i = (int)path.length() - 1; i >= 1; --i) {
      if (path[i] == CHAR_FORWARD_SLASH) {
        if (!matchedSlash) {
          end = i;
          break;
        }
      } else {
        matchedSlash = false;
      }
    }

    if (end == -1)
      return hasRoot ? "/" : ".";
    if (hasRoot && end == 1)
      return "//";
    return toyo::charset::w2a(toyo::string::wslice(path, 0, end));
  }

  std::string to_namespaced_path(const std::string& path) {
    return path;
  }

  std::string basename(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    int start = 0;
    int end = -1;
    bool matchedSlash = true;
    int i;

    for (i = (int)path.length() - 1; i >= 0; --i) {
      if (path[i] == CHAR_FORWARD_SLASH) {
        if (!matchedSlash) {
          start = i + 1;
          break;
        }
      } else if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
    }

    if (end == -1)
      return "";
    return toyo::charset::w2a(toyo::string::wslice(path, start, end));
  }

  std::string basename(const std::string& p, const std::string& e) {
    std::wstring path = toyo::charset::a2w(p);
    std::wstring ext = toyo::charset::a2w(e);

    int start = 0;
    int end = -1;
    bool matchedSlash = true;
    int i;

    if (ext.length() > 0 && ext.length() <= path.length()) {
      if (ext.length() == path.length() && ext == path)
        return "";
      int extIdx = (int)ext.length() - 1;
      int firstNonSlashEnd = -1;
      for (i = (int)path.length() - 1; i >= 0; --i) {
        const unsigned short code = path[i];
        if (code == CHAR_FORWARD_SLASH) {
          if (!matchedSlash) {
            start = i + 1;
            break;
          }
        } else {
          if (firstNonSlashEnd == -1) {
            matchedSlash = false;
            firstNonSlashEnd = i + 1;
          }
          if (extIdx >= 0) {
            if (code == ext[extIdx]) {
              if (--extIdx == -1) {
                end = i;
              }
            } else {
              extIdx = -1;
              end = firstNonSlashEnd;
            }
          }
        }
      }

      if (start == end)
        end = firstNonSlashEnd;
      else if (end == -1)
        end = (int)path.length();
      return toyo::charset::w2a(toyo::string::wslice(path, start, end));
    }

    return posix::basename(p);
  }

  std::string extname(const std::string& p) {
    std::wstring path = toyo::charset::a2w(p);
    int startDot = -1;
    int startPart = 0;
    int end = -1;
    bool matchedSlash = true;
    int preDotState = 0;
    for (int i = (int)path.length() - 1; i >= 0; --i) {
      const unsigned short code = path[i];
      if (code == CHAR_FORWARD_SLASH) {
        if (!matchedSlash) {
          startPart = i + 1;
          break;
        }
        continue;
      }
      if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
      if (code == CHAR_DOT) {
        if (startDot == -1)
          startDot = i;
        else if (preDotState != 1)
          preDotState = 1;
      } else if (startDot != -1) {
        preDotState = -1;
      }
    }

    if (startDot == -1 ||
      end == -1 ||
      preDotState == 0 ||
      (preDotState == 1 &&
        startDot == end - 1 &&
        startDot == startPart + 1)) {
      return "";
    }
    return toyo::charset::w2a(toyo::string::wslice(path, startDot, end));
  }

  std::string relative(const std::string& f, const std::string& t) {
    std::wstring from = toyo::charset::a2w(f);
    std::wstring to = toyo::charset::a2w(t);

    if (from == to)
      return "";

    from = toyo::charset::a2w(posix::resolve(f));
    to = toyo::charset::a2w(posix::resolve(t));

    if (from == to)
      return "";

    int fromStart = 1;
    for (; fromStart < (int)from.length(); ++fromStart) {
      if (from[fromStart] != CHAR_FORWARD_SLASH)
        break;
    }
    int fromEnd = (int)from.length();
    int fromLen = (fromEnd - fromStart);

    int toStart = 1;
    for (; toStart < (int)to.length(); ++toStart) {
      if (to[toStart] != CHAR_FORWARD_SLASH)
        break;
    }
    int toEnd = (int)to.length();
    int toLen = (toEnd - toStart);

    // Compare paths to find the longest common path from root
    int length = (fromLen < toLen ? fromLen : toLen);
    int lastCommonSep = -1;
    int i = 0;
    for (; i <= length; ++i) {
      if (i == length) {
        if (toLen > length) {
          if (to[toStart + i] == CHAR_FORWARD_SLASH) {
            return toyo::charset::w2a(toyo::string::wslice(to, toStart + i + 1));
          } else if (i == 0) {
            return toyo::charset::w2a(toyo::string::wslice(to, toStart + i));
          }
        } else if (fromLen > length) {
          if (from[fromStart + i] == CHAR_FORWARD_SLASH) {
            lastCommonSep = i;
          } else if (i == 0) {
            lastCommonSep = 0;
          }
        }
        break;
      }
      int fromCode = from[fromStart + i];
      int toCode = to[toStart + i];
      if (fromCode != toCode)
        break;
      else if (fromCode == CHAR_FORWARD_SLASH)
        lastCommonSep = i;
    }

    std::wstring out = L"";
    for (i = fromStart + lastCommonSep + 1; i <= fromEnd; ++i) {
      if (i == fromEnd || from[i] == CHAR_FORWARD_SLASH) {
        if (out.length() == 0)
          out += L"..";
        else
          out += L"/..";
      }
    }

    if (out.length() > 0)
      return toyo::charset::w2a(out + toyo::string::wslice(to, toStart + lastCommonSep));
    else {
      toStart += lastCommonSep;
      if (to[toStart] == CHAR_FORWARD_SLASH)
        ++toStart;
      return toyo::charset::w2a(toyo::string::wslice(to, toStart));
    }
  }
} // posix

std::string normalize(const std::string& p) {
#ifdef _WIN32
  return win32::normalize(p);
#else
  return posix::normalize(p);
#endif
}

bool is_absolute(const std::string& p) {
#ifdef _WIN32
  return win32::is_absolute(p);
#else
  return posix::is_absolute(p);
#endif
}

std::string dirname(const std::string& p) {
#ifdef _WIN32
  return win32::dirname(p);
#else
  return posix::dirname(p);
#endif
}

std::string to_namespaced_path(const std::string& p) {
#ifdef _WIN32
  return win32::to_namespaced_path(p);
#else
  return posix::to_namespaced_path(p);
#endif
}

std::string basename(const std::string& p) {
#ifdef _WIN32
  return win32::basename(p);
#else
  return posix::basename(p);
#endif
}

std::string basename(const std::string& p, const std::string& ext) {
#ifdef _WIN32
  return win32::basename(p, ext);
#else
  return posix::basename(p, ext);
#endif
}

std::string extname(const std::string& p) {
#ifdef _WIN32
  return win32::extname(p);
#else
  return posix::extname(p);
#endif
}

std::string relative(const std::string& f, const std::string& t) {
#ifdef _WIN32
  return win32::relative(f, t);
#else
  return posix::relative(f, t);
#endif
}

std::string __filename() {
#if defined(_WIN32)
  wchar_t buf[MAX_PATH] = { 0 };
  DWORD code = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  if (code == 0) return "";
  return toyo::charset::w2a(buf);
#elif defined(__APPLE__)
  char buf[1024] = { 0 };
  unsigned size = 1023;
  int code = _NSGetExecutablePath(buf, &size);
  if (code != 0) return "";
  return posix::normalize(buf);
#else
  char buf[1024] = { 0 };
  int code = readlink("/proc/self/exe", buf, 1023);
  if (code <= 0) return "";
  return buf;
#endif
}

std::string __dirname() {
  return dirname(__filename());
}

std::string tmpdir() {
  std::string path;
  std::map<std::string, std::string> env = process::env();
#ifdef _WIN32
  if (env.find("TEMP") != env.end() && env["TEMP"] != "") {
    path = env["TEMP"];
  } else if (env.find("TMP") != env.end() && env["TMP"] != "") {
    path = env["TMP"];
  } else if (env.find("SystemRoot") != env.end() && env["SystemRoot"] != "") {
    path = env["SystemRoot"] + "\\temp";
  } else if (env.find("windir") != env.end() && env["windir"] != "") {
    path = env["windir"] + "\\temp";
  } else {
    path = "C:\\temp";
  }

  std::wstring wpath = toyo::charset::a2w(path);
  if (wpath.length() > 1 && wpath[wpath.length() - 1] == L'\\' && wpath[wpath.length() - 2] != L':') {
    wpath = toyo::string::wslice(wpath, 0, -1);
    path = toyo::charset::w2a(wpath);
  }
#else
  if (env.find("TMPDIR") != env.end() && env["TMPDIR"] != "") {
    path = env["TMPDIR"];
  } else if (env.find("TMP") != env.end() && env["TMP"] != "") {
    path = env["TMP"];
  } else if (env.find("TEMP") != env.end() && env["TEMP"] != "") {
    path = env["TEMP"];
  } else {
    path = "/tmp";
  }

  std::wstring wpath = toyo::charset::a2w(path);
  if (wpath.length() > 1 && wpath[wpath.length() - 1] == L'/') {
    wpath = toyo::string::wslice(wpath, 0, -1);
    path = toyo::charset::w2a(wpath);
  }
#endif
  return path;
}

std::string homedir() {
  std::map<std::string, std::string> env = process::env();
#ifdef _WIN32

  if (env.find("USERPROFILE") != env.end()) {
    return env.at("USERPROFILE");
  }

  HANDLE token;
  if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token) == 0) {
    throw std::runtime_error(get_win32_last_error_message());
  }
  
  wchar_t path[MAX_PATH];
  DWORD bufsize = sizeof(path) / sizeof(path[0]);

  if (!GetUserProfileDirectoryW(token, path, &bufsize)) {
    CloseHandle(token);
    throw std::runtime_error(get_win32_last_error_message());
  }

  CloseHandle(token);

  return toyo::charset::w2a(path);
#else
  if (env.find("HOME") != env.end()) {
    return env.at("HOME");
  }

  struct passwd pw;
  struct passwd* result;
  std::size_t bufsize;
#if defined(__ANDROID_API__) && __ANDROID_API__ < 21
  int (*getpwuid_r)(uid_t, struct passwd*, char*, size_t, struct passwd**);

  getpwuid_r = dlsym(RTLD_DEFAULT, "getpwuid_r");
  if (getpwuid_r == NULL) {
    throw std::runtime_error("Can not call getpwuid_r.");
  }
#endif
  long initsize = sysconf(_SC_GETPW_R_SIZE_MAX);

  if (initsize <= 0)
    bufsize = 4096;
  else
    bufsize = (size_t) initsize;

  uid_t uid = geteuid();
  char* buf = (char*)malloc(bufsize);
  int r;
  
  for (;;) {
    free(buf);
    buf = (char*)malloc(bufsize);

    if (buf == NULL) {
      throw cerror(errno);
    }

    r = getpwuid_r(uid, &pw, buf, bufsize, &result);

    if (r != ERANGE)
      break;

    bufsize *= 2;
  }

  if (r != 0) {
    free(buf);
    throw std::runtime_error("getpwuid_r() failed.");
  }

  if (result == NULL) {
    free(buf);
    throw cerror(ENOENT);
  }

  return pw.pw_dir;
#endif
}

path::path(): dir_(""), root_(""), base_(""), name_(""), ext_("") {}

#ifdef _WIN32
path::path(const std::string& p): path(p, true) {}
path::path(const char* p): path(std::string(p), true) {}
#else
path::path(const std::string& p): path(p, false) {}
path::path(const char* p): path(std::string(p), false) {}
#endif

path::path(const std::string& p, bool is_win32): path() {
  if (is_win32) {
    std::wstring path = toyo::charset::a2w(p);

    if (path.length() == 0) {
      return;
    }

    int len = (int)path.length();
    int rootEnd = 0;
    unsigned short code = path[0];

    if (len > 1) {
      if (_isPathSeparator(code)) {

        rootEnd = 1;
        if (_isPathSeparator(path[1])) {
          int j = 2;
          int last = j;
          for (; j < len; ++j) {
            if (_isPathSeparator(path[j]))
              break;
          }
          if (j < len && j != last) {
            last = j;
            for (; j < len; ++j) {
              if (!_isPathSeparator(path[j]))
                break;
            }
            if (j < len && j != last) {
              last = j;
              for (; j < len; ++j) {
                if (_isPathSeparator(path[j]))
                  break;
              }
              if (j == len) {
                rootEnd = j;
              } else if (j != last) {
                rootEnd = j + 1;
              }
            }
          }
        }
      } else if (_isWindowsDeviceRoot(code)) {

        if (path[1] == CHAR_COLON) {
          rootEnd = 2;
          if (len > 2) {
            if (_isPathSeparator(path[2])) {
              if (len == 3) {
                std::string apath = toyo::charset::w2a(path);
                this->root_ = this->dir_ = apath;
                return;
              }
              rootEnd = 3;
            }
          } else {
            std::string apath = toyo::charset::w2a(path);
            this->root_ = this->dir_ = apath;
            return;
          }
        }
      }
    } else if (_isPathSeparator(code)) {
      std::string apath = toyo::charset::w2a(path);
      this->root_ = this->dir_ = apath;
      return;
    }

    if (rootEnd > 0) {
      this->root_ = toyo::charset::w2a(toyo::string::wslice(path, 0, rootEnd));
    }

    int startDot = -1;
    int startPart = rootEnd;
    int end = -1;
    bool matchedSlash = true;
    int i = (int)path.length() - 1;

    int preDotState = 0;

    for (; i >= rootEnd; --i) {
      code = path[i];
      if (_isPathSeparator(code)) {
        if (!matchedSlash) {
          startPart = i + 1;
          break;
        }
        continue;
      }
      if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
      if (code == CHAR_DOT) {
        if (startDot == -1)
          startDot = i;
        else if (preDotState != 1)
          preDotState = 1;
      } else if (startDot != -1) {
        preDotState = -1;
      }
    }

    if (startDot == -1 ||
      end == -1 ||
      preDotState == 0 ||
      (preDotState == 1 &&
        startDot == end - 1 &&
        startDot == startPart + 1)) {
      if (end != -1) {
        std::string tmp = toyo::charset::w2a(toyo::string::wslice(path, startPart, end));
        this->base_ = this->name_ = tmp;
      }
    } else {
      this->name_ = toyo::charset::w2a(toyo::string::wslice(path, startPart, startDot));
      this->base_ = toyo::charset::w2a(toyo::string::wslice(path, startPart, end));
      this->ext_ = toyo::charset::w2a(toyo::string::wslice(path, startDot, end));
    }

    if (startPart > 0 && startPart != rootEnd)
      this->dir_ = toyo::charset::w2a(toyo::string::wslice(path, 0, startPart - 1));
    else
      this->dir_ = this->root_;
  } else {
    std::wstring path = toyo::charset::a2w(p);
    if (path.length() == 0) {
      return;
    }
    bool isAbsolute = (path[0] == CHAR_FORWARD_SLASH);
    int start;
    if (isAbsolute) {
      this->root_ = "/";
      start = 1;
    } else {
      start = 0;
    }
    int startDot = -1;
    int startPart = 0;
    int end = -1;
    bool matchedSlash = true;
    int i = (int)path.length() - 1;

    int preDotState = 0;

    for (; i >= start; --i) {
      const unsigned short code = path[i];
      if (code == CHAR_FORWARD_SLASH) {
        if (!matchedSlash) {
          startPart = i + 1;
          break;
        }
        continue;
      }
      if (end == -1) {
        matchedSlash = false;
        end = i + 1;
      }
      if (code == CHAR_DOT) {
        if (startDot == -1)
          startDot = i;
        else if (preDotState != 1)
          preDotState = 1;
      } else if (startDot != -1) {
        preDotState = -1;
      }
    }

    if (startDot == -1 ||
      end == -1 ||
      preDotState == 0 ||
      (preDotState == 1 &&
        startDot == end - 1 &&
        startDot == startPart + 1)) {
      if (end != -1) {
        if (startPart == 0 && isAbsolute) {
          std::string tmp = toyo::charset::w2a(toyo::string::wslice(path, 1, end));
          this->base_ = this->name_ = tmp;
        } else {
          std::string tmp = toyo::charset::w2a(toyo::string::wslice(path, startPart, end));
          this->base_ = this->name_ = tmp;
        }
      }
    } else {
      if (startPart == 0 && isAbsolute) {
        this->name_ = toyo::charset::w2a(toyo::string::wslice(path, 1, startDot));
        this->base_ = toyo::charset::w2a(toyo::string::wslice(path, 1, end));
      } else {
        this->name_ = toyo::charset::w2a(toyo::string::wslice(path, startPart, startDot));
        this->base_ = toyo::charset::w2a(toyo::string::wslice(path, startPart, end));
      }
      this->ext_ = toyo::charset::w2a(toyo::string::wslice(path, startDot, end));
    }

    if (startPart > 0)
      this->dir_ = toyo::charset::w2a(toyo::string::wslice(path, 0, startPart - 1));
    else if (isAbsolute)
      this->dir_ = "/";
  }
}

path path::parse_win32(const std::string& p) { return path(p, true); }

path path::parse_posix(const std::string& p) { return path(p, false); }

path path::parse(const std::string& p) { return path(p); }

std::string path::dir() const { return this->dir_; }
std::string path::root() const { return this->root_; }
std::string path::base() const { return this->base_; }
std::string path::name() const { return this->name_; }
std::string path::ext() const { return this->ext_; }

path& path::dir(const std::string& d) { this->dir_ = d; return *this; }
path& path::root(const std::string& r) { this->root_ = r; return *this; }
path& path::base(const std::string& b) { this->base_ = b; return *this; }
path& path::name(const std::string& n) { this->name_ = n; return *this; }
path& path::ext(const std::string& e) { this->ext_ = e; return *this; }

// path& path::operator=(const std::string& other) {
//   return this->operator=(path(other));
// }

path path::operator+(const path& p) const {
#ifdef _WIN32
  return path::parse_win32(win32::join(format_win32(), p.format_win32()));
#else
  return path::parse_posix(posix::join(format_posix(), p.format_posix()));
#endif
}

path& path::operator+=(const path& p) {
#ifdef _WIN32
  *this = path::parse_win32(win32::join(format_win32(), p.format_win32()));
#else
  *this = path::parse_posix(posix::join(format_posix(), p.format_posix()));
#endif
  return *this;
}

std::string path::_format(const std::string& sep) const {
  const std::string& dir = (this->dir_ == "" ?  this->root_ : this->dir_);
  const std::string& base = (this->base_ != "" ? this->base_ :
    (this->name_ + this->ext_));
  if (dir == "") {
    return base;
  }
  if (dir == this->root_) {
    return dir + base;
  }
  return dir + sep + base;
}

std::string path::format_win32() const {
  return this->_format("\\");
}

std::string path::format_posix() const {
  return this->_format("/");
}

std::string path::format() const {
#ifdef _WIN32
  return format_win32();
#else
  return format_posix();
#endif
}

bool path::operator==(const path& other) {
  if (&other == this) {
    return true;
  }
  return (other.dir_ == this->dir_
    && other.root_ == this->root_
    && other.base_ == this->base_
    && other.name_ == this->name_
    && other.ext_ == this->ext_);
}

env_paths::env_paths(const std::string& name):
  data(""),
  config(""),
  cache(""),
  log(""),
  temp("") {
  auto env = process::env();
  auto homedir = toyo::path::homedir();
  auto tmpdir = toyo::path::tmpdir();
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
  std::string appdata;
  std::string local_appdata;
  if (env.find("APPDATA") != env.end() && env.at("APPDATA") != "") {
    appdata = env.at("APPDATA");
  } else {
    appdata = toyo::path::win32::join(homedir, "AppData", "Roaming");
  }

  if (env.find("LOCALAPPDATA") != env.end() && env.at("LOCALAPPDATA") != "") {
    local_appdata = env.at("LOCALAPPDATA");
  } else {
    local_appdata = toyo::path::win32::join(homedir, "AppData", "Local");
  }

  this->data = toyo::path::win32::join(local_appdata, name, "Data");
  this->config = toyo::path::win32::join(appdata, name, "Config");
  this->cache = toyo::path::win32::join(local_appdata, name, "Cache");
  this->log = toyo::path::win32::join(local_appdata, name, "Log");
  this->temp = toyo::path::win32::join(tmpdir, name);
#elif defined(__APPLE__)
  auto library = toyo::path::posix::join(homedir, "Library");

  this->data = toyo::path::posix::join(library, "Application Support", name);
  this->config = toyo::path::posix::join(library, "Preferences", name);
  this->cache = toyo::path::posix::join(library, "Caches", name);
  this->log = toyo::path::posix::join(library, "Logs", name);
  this->temp = toyo::path::posix::join(tmpdir, name);
#elif defined(__linux__) || defined(__linux)
  auto username = toyo::path::posix::basename(homedir);

  this->data = toyo::path::posix::join(
    (env.find("XDG_DATA_HOME") != env.end() && env.at("XDG_DATA_HOME") != "") ? env.at("XDG_DATA_HOME") : toyo::path::posix::join(homedir, ".local/share"),
    name);
  this->config = toyo::path::posix::join(
    (env.find("XDG_CONFIG_HOME") != env.end() && env.at("XDG_CONFIG_HOME") != "") ? env.at("XDG_CONFIG_HOME") : toyo::path::posix::join(homedir, ".config"),
    name);
  this->cache = toyo::path::posix::join(
    (env.find("XDG_CACHE_HOME") != env.end() && env.at("XDG_CACHE_HOME") != "") ? env.at("XDG_CACHE_HOME") : toyo::path::posix::join(homedir, ".cache"),
    name);
  this->log = toyo::path::posix::join(
    (env.find("XDG_STATE_HOME") != env.end() && env.at("XDG_STATE_HOME") != "") ? env.at("XDG_STATE_HOME") : toyo::path::posix::join(homedir, ".local/state"),
    name);
  this->temp = toyo::path::posix::join(tmpdir, username, name);
#endif
}

env_paths env_paths::create(const std::string& name, const env_paths_param& param) {
  std::string name_ = name;
  if (param.suffix != "") {
    name_ += ("-" + param.suffix);
  }
  return env_paths(name_);
}

env_paths env_paths::create(const std::string& name) {
  env_paths_param param;
  param.suffix = "";
  return create(name, param);
}

globrex::globrex_options::globrex_options():
  extended(false),
  globstar(false),
  strict(false),
  filepath(false) {}

globrex::globrex(const std::string& glob) {
  globrex_options options;
  this->_init(glob, options);
}

globrex::globrex(const std::string& glob, const globrex_options& options) {
  this->_init(glob, options);
}

bool globrex::filepath() const {
  return this->_filepath;
}

void globrex::_init(const std::string& glob, const globrex_options& options) {
#ifdef _WIN32
  const bool isWin = true;
  const std::string SEP = "(?:\\\\|\\/)";
  const std::string SEP_ESC = "\\\\";
  const std::string SEP_RAW = "\\";
#else
  const bool isWin = false;
  const std::string SEP = "\\/";
  const std::string SEP_ESC = "/";
  const std::string SEP_RAW = "/";
#endif

  const std::string GLOBSTAR = "(?:(?:[^" + SEP_ESC + "/]*(?:" + SEP_ESC + "|/|$))*)";
  const std::string WILDCARD = "(?:[^" + SEP_ESC + "/]*)";
  const std::string GLOBSTAR_SEGMENT = "((?:[^" + SEP_ESC + "/]*(?:" + SEP_ESC + "|/|$))*)";
  const std::string WILDCARD_SEGMENT = "(?:[^" + SEP_ESC + "/]*)";

  bool extended = options.extended;
  bool globstar = options.globstar;
  bool strict = options.strict;
  bool filepath = options.filepath;
  this->_filepath = filepath;

  auto sepPattern = std::regex("^" + SEP + (strict ? "" : "+") + "$");
  std::string regex = "";
  std::string segment = "";
  std::string pathRegexStr = "";
  std::vector<std::regex> pathSegments = {};
  std::vector<std::string> pathSegmentsStr = {};

  auto inGroup = false;
  auto inRange = false;

  std::vector<std::string> ext = {};

  auto add = [&](const std::string& str, const add_options& options) {
    auto split = options.split;
    auto last = options.last;
    auto only = options.only;
    if (only != "path") regex += str;
    if (filepath && only != "regex") {
      pathRegexStr += std::regex_search(str, sepPattern) ? SEP : str;
      if (split) {
        if (last) segment += str;
        if (segment != "") {
          // change it 'includes'
          segment = "^" + segment + "$";
          pathSegments.push_back(std::regex(segment));
          pathSegmentsStr.push_back(segment);
        }
        segment = "";
      } else {
        segment += str;
      }
    }
  };

  std::wstring wglob = toyo::charset::a2w(glob);
  std::wstring c, n;
  for (size_t i = 0; i < wglob.size(); i++) {
    c = wglob[i];
    n = ((i + 1) == wglob.size()) ? std::wstring(L"") : std::wstring({ wglob[i + 1] });

    add_options opts;
    opts.split = false;
    opts.last = false;
    opts.only = "";

    if (c == L"\\" || c == L"$" || c == L"^" || c == L"." || c == L"=") {
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (std::regex_search(toyo::charset::w2a(c), sepPattern)) {
      opts.split = true;
      add(SEP, opts);
      if (n != L"" && std::regex_search(toyo::charset::w2a(n), sepPattern) && !strict) regex += "?";
      continue;
    }

    if (c == L"(") {
      if (ext.size() > 0) {
        add(toyo::charset::w2a(c) + "?:", opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L")") {
      if (ext.size() > 0) {
        add(toyo::charset::w2a(c), opts);
        std::string type = ext.back();
        ext.pop_back();
        if (type == "@") {
          add("{1}", opts);
        } else if (type == "!") {
          add(WILDCARD, opts);
        } else {
          add(type, opts);
        }
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"|") {
      if (ext.size() > 0) {
        add(toyo::charset::w2a(c), opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"+") {
      if (n == L"(" && extended) {
        ext.push_back(toyo::charset::w2a(c));
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"@" && extended) {
      if (n == L"(") {
        ext.push_back(toyo::charset::w2a(c));
        continue;
      }
    }

    if (c == L"!") {
      if (extended) {
        if (inRange) {
          add("^", opts);
          continue;
        }
        if (n == L"(") {
          ext.push_back(toyo::charset::w2a(c));
          add("(?!", opts);
          i++;
          continue;
        }
        add("\\" + toyo::charset::w2a(c), opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"?") {
      if (extended) {
        if (n == L"(") {
          ext.push_back(toyo::charset::w2a(c));
        } else {
          add(".", opts);
        }
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"[") {
      if (inRange && n == L":") {
        i++; // skip [
        std::wstring value = L"";
        i++;
        while (wglob[i] != L':') {
          value += wglob[i];
          i++;
        }
        if (value == L"alnum") add("(?:\\w|\\d)", opts);
        else if (value == L"space") add("\\s", opts);
        else if (value == L"digit") add("\\d", opts);
        i++; // skip last ]
        continue;
      }
      if (extended) {
        inRange = true;
        add(toyo::charset::w2a(c), opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"]") {
      if (extended) {
        inRange = false;
        add(toyo::charset::w2a(c), opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"{") {
      if (extended) {
        inGroup = true;
        add("(?:", opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"}") {
      if (extended) {
        inGroup = false;
        add(")", opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L",") {
      if (inGroup) {
        add("|", opts);
        continue;
      }
      add("\\" + toyo::charset::w2a(c), opts);
      continue;
    }

    if (c == L"*") {
      if (n == L"(" && extended) {
        ext.push_back(toyo::charset::w2a(c));
        continue;
      }
      // Move over all consecutive "*"'s.
      // Also store the previous and next characters
      std::string prevChar;
      if (i == 0) {
        prevChar = "";
      } else {
        prevChar = toyo::charset::w2a(std::wstring({ wglob[i - 1] }));
      }
      size_t starCount = 1;
      while (wglob[i + 1] == L'*') {
        starCount++;
        i++;
      }
      std::string nextChar;
      if (i + 1 == wglob.size()) {
        nextChar = "";
      } else {
        nextChar = toyo::charset::w2a(std::wstring({ wglob[i + 1] }));
      }
      if (!globstar) {
        // globstar is disabled, so treat any number of "*" as one
        add(".*", opts);
      } else {
        // globstar is enabled, so determine if this is a globstar segment
        bool isGlobstar =
          starCount > 1 && // multiple "*"'s
          // from the start of the segment
          (prevChar == SEP_RAW || prevChar == "/" || prevChar == "") &&
          (nextChar == SEP_RAW || nextChar == "/" || nextChar == "");
        if (isGlobstar) {
          // it's a globstar, so match zero or more path segments
          add_options opt1;
          opt1.last = false;
          opt1.split = false;
          opt1.only = "regex";
          add(GLOBSTAR, opt1);
          add_options opt2;
          opt2.last = true;
          opt2.split = true;
          opt2.only = "path";
          add(GLOBSTAR_SEGMENT, opt2);
          i++; // move over the "/"
        } else {
          // it's not a globstar, so only match one path segment
          add_options opt1;
          opt1.last = false;
          opt1.split = false;
          opt1.only = "regex";
          add(WILDCARD, opt1);
          add_options opt2;
          opt2.last = false;
          opt2.split = false;
          opt2.only = "path";
          add(WILDCARD_SEGMENT, opt2);
        }
      }
      continue;
    }

    add(toyo::charset::w2a(c), opts);
  }

  regex = "^" + regex + "$";
  segment = "^" + segment + "$";
  if (filepath) {
    pathRegexStr = "^" + pathRegexStr + "$";
  }

  this->regex = std::regex(regex);
  this->regex_str = regex;

  if (filepath) {
    pathSegments.push_back(std::regex(segment));
    pathSegmentsStr.push_back(segment);
    this->path_regex = std::regex(pathRegexStr);
    this->path_regex_str = pathRegexStr;
    this->path_segments = pathSegments;
    this->path_segments_str = pathSegmentsStr;
    this->path_globstar_str = "^" + GLOBSTAR_SEGMENT + "$";
    this->path_globstar = std::regex(this->path_globstar_str);
  }
}

std::regex globrex::glob_to_regex(const std::string& glob) {
  globrex_options options;
  options.extended = true;
  options.filepath = true;
  options.strict = false;
  options.globstar = true;
  return globrex(glob, options).path_regex;
}

bool globrex::match(const std::string& str, const std::string& glob, const globrex_options* opts) {
  std::regex re;
  try {
    if (opts == nullptr) {
      re = glob_to_regex(glob);
    } else {
      re = globrex(glob, *opts).path_regex;
    }
  } catch (const std::exception&) {
    return false;
  }
  // std::vector<std::smatch> match;
  size_t matches = 0;
  std::smatch sm;
  std::string tmpstr = str;
  while(tmpstr != "" && std::regex_search(tmpstr, sm, re)) {
    matches++;
    tmpstr = sm.suffix();
  }

  return matches == 1;
}

} // path

} // toyo
