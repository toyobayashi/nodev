#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <string>
#include <vector>
#include <stdexcept>
#include "toyo/util.hpp"

namespace toyo {

namespace util {

md5::~md5() {
  if (hash_) {
    md5_free(hash_);
    hash_ = nullptr;
  }
}

md5::md5(): hash_(md5_init()) {
  if (hash_ == nullptr) {
    throw std::runtime_error(md5_get_error_message(md5_get_last_error()));
  }
}

md5::md5(const md5& other): md5() { md5_copy(hash_, other.hash_); }

md5::md5(md5&& other) {
  hash_ = other.hash_;
  other.hash_ = nullptr;
}
md5& md5::operator=(md5&& other) {
  hash_ = other.hash_;
  other.hash_ = nullptr;
  return *this;
}

md5& md5::operator=(const md5& other) {
  if (this == &other) {
    return *this;
  }
  md5_copy(hash_, other.hash_);
  return *this;
};

bool md5::operator==(const md5& other) const {
  if (this == &other) {
    return true;
  }
  int result = 0;
  int r = md5_cmp(hash_, other.hash_, &result);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return result == 0;
}

bool md5::operator!=(const md5& other) const {
  return !(*this == other);
}

bool md5::operator<(const md5& other) const {
  if (this == &other) {
    return false;
  }
  int result = 0;
  int r = md5_cmp(hash_, other.hash_, &result);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return result == -1;
}

bool md5::operator>(const md5& other) const {
  if (this == &other) {
    return false;
  }
  int result = 0;
  int r = md5_cmp(hash_, other.hash_, &result);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return result == 1;
}

bool md5::operator<=(const md5& other) const {
  if (this == &other) {
    return true;
  }
  int result = 0;
  int r = md5_cmp(hash_, other.hash_, &result);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return result == -1 || result == 0;
}

bool md5::operator>=(const md5& other) const {
  if (this == &other) {
    return true;
  }
  int result = 0;
  int r = md5_cmp(hash_, other.hash_, &result);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return result == 1 || result == 0;
}

void md5::update(const uint8_t* data, int length) {
  int r = md5_update(hash_, data, length);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
}

void md5::update(const std::string& data) {
  int r = md5_update(hash_, (const unsigned char*)data.c_str(), (int)data.length());
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
}

void md5::update(const std::vector<uint8_t>& data) {
  int r = md5_update(hash_, (const unsigned char*)data.data(), (int)data.size());
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
}

std::string md5::digest() {
  char hex[33];
  int r = md5_digest(hash_, hex);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return hex;
}

void md5::swap(md5& other) {
  md5_hash* tmp = other.hash_;
  other.hash_ = hash_;
  hash_ = tmp;
}

const md5_hash* md5::data() const { return hash_; }

std::string md5::calc_str(const std::string& msg) {
  char res[33];
  int r = ::md5(msg.c_str(), &res[0]);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return res;
}

std::string md5::calc_file(const std::string& path) {
  char res[33];
  int r = md5_file(path.c_str(), &res[0]);
  if (r != 0) {
    throw std::runtime_error(md5_get_error_message(r));
  }
  return res;
}

}

}
