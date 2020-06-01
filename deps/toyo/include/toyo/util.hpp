#ifndef __TOYO_UTIL_HPP__
#define __TOYO_UTIL_HPP__

#include <string>
#include <vector>
#include "./util/sha256.h"
#include "./util/md5.h"

namespace toyo {

namespace util {
  std::string to_b64(const std::string& buffer);
  std::string to_b64(const std::vector<unsigned char>& buffer);

  std::vector<unsigned char> b64_to_buffer(const std::string& b64);
  std::string b64_to_string(const std::string& b64);
  class sha256 {
  public:
    ~sha256();
    sha256();
    sha256(const sha256&);
    sha256(sha256&&);
    sha256& operator=(sha256&&);
    sha256& operator=(const sha256&);

    bool operator==(const sha256& other) const;
    bool operator!=(const sha256& other) const;
    bool operator<(const sha256& other) const;
    bool operator>(const sha256& other) const;
    bool operator<=(const sha256& other) const;
    bool operator>=(const sha256& other) const;

    void update(const uint8_t* data, int length);
    void update(const std::string& data);
    void update(const std::vector<uint8_t>& data);

    std::string digest();

    void swap(sha256& other);

    const ::sha256_hash* data() const;

    static std::string calc_str(const std::string& msg);

    static std::string calc_file(const std::string& path);
  private:
    ::sha256_hash* hash_;
  };

  class md5 {
   public:
    ~md5();
    md5();
    md5(const md5&);
    md5(md5&&);

    md5& operator=(md5&&);
    md5& operator=(const md5&);
    bool operator==(const md5& other) const;
    bool operator!=(const md5& other) const;
    bool operator<(const md5& other) const;
    bool operator>(const md5& other) const;
    bool operator<=(const md5& other) const;
    bool operator>=(const md5& other) const;

    void update(const uint8_t* data, int length);
    void update(const std::string& data);
    void update(const std::vector<uint8_t>& data);

    std::string digest();

    void swap(md5& other);

    const ::md5_hash* data() const;

    static std::string calc_str(const std::string& msg);
    static std::string calc_file(const std::string& path);
   private:
    ::md5_hash* hash_;
  };

}

}

#endif
