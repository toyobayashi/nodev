#include "cli.hpp"
#include "toyo/charset.hpp"
#include <cstring>

namespace nodev {

cli::cli(int argc, char** argv) {
  if (argc < 2) {
    return;
  }

  _command = argv[1];

  if (argc < 3) {
    return;
  }

  std::string arg = "";

  for (int i = 2; i < argc; i++) {
    arg = argv[i];
    if (arg.find("--") == 0) {
      arg = arg.substr(2);
      auto index = arg.find("=");
      if (index == std::string::npos) {
        _option[arg] = "true";
      } else {
        auto key = arg.substr(0, index);
        auto value = arg.substr(index + 1);
        _option[key] = value;
      }
    } else {
      _.push_back(arg);
    }
  }
}

cli::cli(int argc, wchar_t** argv) {
  char** argv_ = new char*[argc];
  for (int i = 0; i < argc; i++) {
    std::string arg = toyo::charset::w2a(argv[i]);
    std::size_t size = arg.length() + 1;
    argv_[i] = new char[size];
    memset(argv_[i], 0, size);
    strcpy(argv_[i], arg.c_str());
  }

  new (this)cli(argc, argv_);

  for (int i = 0; i < argc; i++) {
    delete[] argv_[i];
  }

  delete[] argv_;
}

bool cli::has(const std::string& optionName) const {
  auto it = _option.find(optionName);
  if (it == _option.end()) {
    return false;
  }
  return true;
}

const std::string& cli::get_command() const {
  return _command;
}

std::string cli::get_option(const std::string& optionName) const {
  if (has(optionName)) {
    return _option.at(optionName);
  }
  return "";
}

const std::vector<std::string>& cli::get_argument() const {
  return _;
}

void cli::show_detail() const {
  printf("Command: %s\n", _command.c_str());

  for (auto iter = _option.begin(); iter != _option.end(); iter++) {
    printf("Option: %s = %s\n", iter->first.c_str(), iter->second.c_str());
  }
  
  for (unsigned int i = 0; i < _.size(); i++) {
    printf("Arguments: %s\n", _[i].c_str());
  }
}

}
