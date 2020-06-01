#include "toyo/util/base64.h"
#include "toyo/util.hpp"

namespace toyo {

namespace util {
  std::string to_b64(const std::string& buffer) {
    const unsigned char* src = reinterpret_cast<const unsigned char*>(buffer.c_str());
    std::size_t slen = buffer.length();
    std::size_t dlen = ::base64_encode(src, slen, nullptr);
    if (dlen == 0) return "";
    std::string res;
    res.resize(dlen);
    ::base64_encode(src, slen, &res[0]);
    return res;
  }
  std::string to_b64(const std::vector<unsigned char>& buffer) {
    const unsigned char* src = buffer.data();
    std::size_t slen = buffer.size();
    std::size_t dlen = ::base64_encode(src, slen, nullptr);
    if (dlen == 0) return "";
    std::string res;
    res.resize(dlen);
    ::base64_encode(src, slen, &res[0]);
    return res;
  }

  std::vector<unsigned char> b64_to_buffer(const std::string& b64) {
    const char* src = b64.c_str();
    std::size_t slen = b64.length();
    std::size_t dlen = ::base64_decode(src, slen, nullptr);
    if (dlen == 0) return {};
    std::vector<unsigned char> res(dlen);
    ::base64_decode(src, slen, &res[0]);
    return res;
  }
  std::string b64_to_string(const std::string& b64) {
    const char* src = b64.c_str();
    std::size_t slen = b64.length();
    std::size_t dlen = ::base64_decode(src, slen, nullptr);
    if (dlen == 0) return "";
    std::string res;
    res.resize(dlen);
    ::base64_decode(src, slen, reinterpret_cast<unsigned char*>(&res[0]));
    return res;
  }
}
}
