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
  sha256::~sha256() {
    if (hash_) {
      sha256_free(hash_);
      hash_ = nullptr;
    }
  }

  sha256::sha256(): hash_(sha256_init()) {
    if (hash_ == nullptr) {
      throw std::runtime_error(sha256_get_error_message(sha256_get_last_error()));
    }
  }

  sha256::sha256(const sha256& other): sha256() { sha256_copy(hash_, other.hash_); }
  sha256::sha256(sha256&& other) {
    hash_ = other.hash_;
    other.hash_ = nullptr;
  }
  sha256& sha256::operator=(sha256&& other) {
    hash_ = other.hash_;
    other.hash_ = nullptr;
    return *this;
  }

  sha256& sha256::operator=(const sha256& other) {
    if (this == &other) {
      return *this;
    }
    sha256_copy(hash_, other.hash_);
    return *this;
  }

  bool sha256::operator==(const sha256& other) const {
    if (this == &other) {
      return true;
    }
    int result = 0;
    int r = sha256_cmp(hash_, other.hash_, &result);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return result == 0;
  }

  bool sha256::operator!=(const sha256& other) const {
    return !(*this == other);
  }

  bool sha256::operator<(const sha256& other) const {
    if (this == &other) {
      return false;
    }
    int result = 0;
    int r = sha256_cmp(hash_, other.hash_, &result);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return result == -1;
  }

  bool sha256::operator>(const sha256& other) const {
    if (this == &other) {
      return false;
    }
    int result = 0;
    int r = sha256_cmp(hash_, other.hash_, &result);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return result == 1;
  }

  bool sha256::operator<=(const sha256& other) const {
    if (this == &other) {
      return true;
    }
    int result = 0;
    int r = sha256_cmp(hash_, other.hash_, &result);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return result == -1 || result == 0;
  }

  bool sha256::operator>=(const sha256& other) const {
    if (this == &other) {
      return true;
    }
    int result = 0;
    int r = sha256_cmp(hash_, other.hash_, &result);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return result == 1 || result == 0;
  }

  void sha256::update(const uint8_t* data, int length) {
    int r = sha256_update(hash_, data, length);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
  }

  void sha256::update(const std::string& data) {
    int r = sha256_update(hash_, (const unsigned char*)data.c_str(), (int)data.length());
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
  }

  void sha256::update(const std::vector<uint8_t>& data) {
    int r = sha256_update(hash_, (const unsigned char*)data.data(), (int)data.size());
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
  }

  std::string sha256::digest() {
    char hex[65];
    int r = sha256_digest(hash_, hex);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return hex;
  }

  void sha256::swap(sha256& other) {
    sha256_hash* tmp = other.hash_;
    other.hash_ = hash_;
    hash_ = tmp;
  }

  const sha256_hash* sha256::data() const { return hash_; }

  std::string sha256::calc_str(const std::string& msg) {
    char res[65];
    int r = ::sha256(msg.c_str(), &res[0]);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return res;
  }

  std::string sha256::calc_file(const std::string& path) {
    char res[65];
    int r = sha256_file(path.c_str(), res);
    if (r != 0) {
      throw std::runtime_error(sha256_get_error_message(r));
    }
    return res;
  }
}

}
