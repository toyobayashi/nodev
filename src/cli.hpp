#ifndef __NODEV_CLI_HPP__
#define __NODEV_CLI_HPP__

#include <map>
#include <string>
#include <vector>

namespace nodev {

class cli {
private:
  std::string _command;
  std::map<std::string, std::string> _option;
  std::vector<std::string> _;
public:
  cli() = delete;
  cli(const cli&) = delete;
  cli& operator=(const cli&) = delete;
  cli(int argc, char** argv);
  cli(int argc, wchar_t** argv);
  bool has(const std::string&) const;
  const std::string& get_command() const;
  std::string get_option(const std::string&) const;
  const std::vector<std::string>& get_argument() const;
  void show_detail() const;
};

}

#endif
