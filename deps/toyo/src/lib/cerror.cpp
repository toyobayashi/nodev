#include <cstring>

#include "cerror.hpp"

cerror::cerror(int code, const std::string& message): code_(code) {
  if (message != "") {
    this->message_ = std::string(strerror(code_)) + ", " + message;
  } else {
    this->message_ = std::string(strerror(code_)) + ".";
  }
}

const char* cerror::what() const noexcept {
  return this->message_.c_str();
}

int cerror::code() const {
  return code_;
}
