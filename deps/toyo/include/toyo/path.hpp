#ifndef __TOYO_PATH_HPP__
#define __TOYO_PATH_HPP__

#include <string>
#include <regex>
#include <vector>

namespace toyo {

namespace path {

class path {
private:
  std::string dir_;
  std::string root_;
  std::string base_;
  std::string name_;
  std::string ext_;

  std::string _format(const std::string& sep) const;

  path(const std::string&, bool);
public:
  static path parse_win32(const std::string&);
  static path parse_posix(const std::string&);
  static path parse(const std::string&);

  path();
  path(const std::string&);
  path(const char*);

  std::string dir() const;
  std::string root() const;
  std::string base() const;
  std::string name() const;
  std::string ext() const;

  path& dir(const std::string&);
  path& root(const std::string&);
  path& base(const std::string&);
  path& name(const std::string&);
  path& ext(const std::string&);

  path operator+(const path& p) const;
  path& operator+=(const path& p);
  bool operator==(const path& other);
  std::string format_win32() const;
  std::string format_posix() const;
  std::string format() const;
};

namespace win32 {
  const std::string sep = "\\";
  const std::string delimiter = ";";

  std::string normalize(const std::string&);

  std::string resolve(const std::string& arg = "", const std::string& arg1 = "");

  template <typename... Args>
  inline std::string resolve(const std::string& arg1, const std::string& arg2, Args... args) {
    std::string tmp = win32::resolve(arg1, arg2);
    return win32::resolve(tmp, args...);
  }

  std::string join(const std::string&, const std::string& arg2 = "");

  template <typename... Args>
  inline std::string join(const std::string& arg1, const std::string& arg2, Args... args) {
    std::string tmp = win32::join(arg1, arg2);
    return win32::join(tmp, args...);
  }

  bool is_absolute(const std::string&);
  std::string dirname(const std::string&);
  std::string to_namespaced_path(const std::string&);
  std::string basename(const std::string&);
  std::string basename(const std::string&, const std::string&);
  std::string extname(const std::string&);
  std::string relative(const std::string&, const std::string&);
} // win32

namespace posix {
  const std::string sep = "/";
  const std::string delimiter = ":";

  std::string normalize(const std::string&);

  std::string resolve(const std::string& arg = "", const std::string& arg1 = "");

  template <typename... Args>
  inline std::string resolve(const std::string& arg1, const std::string& arg2, Args... args) {
    std::string tmp = posix::resolve(arg1, arg2);
    return posix::resolve(tmp, args...);
  }

  std::string join(const std::string&, const std::string& arg2 = "");

  template <typename... Args>
  inline std::string join(const std::string& arg1, const std::string& arg2, Args... args) {
    std::string tmp = posix::join(arg1, arg2);
    return posix::join(tmp, args...);
  }

  bool is_absolute(const std::string&);
  std::string dirname(const std::string&);
  std::string to_namespaced_path(const std::string&);
  std::string basename(const std::string&);
  std::string basename(const std::string&, const std::string&);
  std::string extname(const std::string&);
  std::string relative(const std::string&, const std::string&);
} // posix

#ifdef _WIN32
  const std::string sep = win32::sep;
  const std::string delimiter = win32::delimiter;
#else
  const std::string sep = posix::sep;
  const std::string delimiter = posix::delimiter;
#endif

std::string normalize(const std::string&);

template <typename... Args>
inline std::string resolve(Args... args) {
#ifdef _WIN32
  return win32::resolve(args...);
#else
  return posix::resolve(args...);
#endif
}

template <typename... Args>
inline std::string join(Args... args) {
#ifdef _WIN32
  return win32::join(args...);
#else
  return posix::join(args...);
#endif
}

bool is_absolute(const std::string&);
std::string dirname(const std::string&);
std::string to_namespaced_path(const std::string&);

std::string basename(const std::string&);
std::string basename(const std::string&, const std::string&);

std::string extname(const std::string&);
std::string relative(const std::string&, const std::string&);

std::string __filename();
std::string __dirname();
std::string tmpdir();
std::string homedir();

typedef struct env_paths_param {
  std::string suffix;
} env_paths_param;

class env_paths {
 private:
  env_paths(const std::string&);
 public:
  static env_paths create(const std::string&);
  static env_paths create(const std::string&, const env_paths_param&);
  std::string data;
  std::string config;
  std::string cache;
  std::string log;
  std::string temp;
};

class globrex {
 public:
  struct globrex_options {
    bool extended;
    bool globstar;
    bool strict;
    bool filepath;
    globrex_options();
  };
  std::regex regex;
  std::string regex_str;
  std::regex path_regex;
  std::string path_regex_str;
  std::vector<std::regex> path_segments;
  std::vector<std::string> path_segments_str;
  std::regex path_globstar;
  std::string path_globstar_str;
  globrex(const std::string&);
  globrex(const std::string&, const globrex_options&);

  bool filepath() const;

  static std::regex glob_to_regex(const std::string&);
  static bool match(const std::string& str, const std::string& glob, const globrex_options* opts = nullptr);
 private:
  bool _filepath;
  void _init(const std::string&, const globrex_options&);
  struct add_options {
    bool split;
    bool last;
    std::string only;
  };
};

} // path

} // toyo

#endif
