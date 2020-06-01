#ifndef __TOYO_CHARSET_HPP__
#define __TOYO_CHARSET_HPP__

#include <string>

namespace toyo {

namespace charset {

std::wstring a2w(const std::string& str);

std::string w2a(const std::wstring& wstr);

std::string w2acp(const std::wstring& wstr);

std::string a2acp(const std::string& str);

std::string w2ocp(const std::wstring& wstr);

std::string a2ocp(const std::string& str);

} // charset

} // toyo

#endif
