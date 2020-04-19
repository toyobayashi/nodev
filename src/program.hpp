#ifndef __NODEV_PROGRAM_HPP__
#define __NODEV_PROGRAM_HPP__

#include <string>
#include "config.hpp"
#include "cli.hpp"

namespace nodev {

class program {
 private:
  nodev_config* config_;
  toyo::path::env_paths* paths_;
  std::string dir;
  std::string root_() const;
  std::string node_cache_dir() const;
  std::string npm_cache_dir() const;
  std::string node_name(const std::string& version) const;
  std::string node_path(const std::string& node_name) const;
  std::string global_node_modules_dir() const;
  static std::string get_node_version(const std::string& exe_path);
  static bool is_x64_executable(const std::string& exe_path);
  static bool is_executable(const std::string& exe_path);
  static std::string get_npm_version(const std::string& node_version);
 public:
  virtual ~program();
  program();
  program(const cli& cli);
  program(const program&) = delete;
  program& operator=(const program&) = delete;
  void version() const;
  void help() const;
  void list() const;
  bool get(const std::string& version) const;
  bool get_npm(const std::string& version) const;
  bool use(const std::string& version) const;
  bool use_npm(const std::string& version) const;
  void rm(const std::string& version) const;
  bool rm_npm() const;
  void node_mirror() const;
  void node_mirror(const std::string& mirror);
  void root() const;
  void root(const std::string& root);
  void npm_mirror() const;
  void npm_mirror(const std::string& mirror);
  void node_arch() const;
  void node_arch(const std::string& arch);
  void node_cache() const;
  void node_cache(const std::string& dir);
  void npm_cache() const;
  void npm_cache(const std::string& dir);
  nodev_config* get_config();
};

}

#endif
