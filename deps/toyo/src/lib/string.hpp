#ifndef __STRING_HPP__
#define __STRING_HPP__

#include <string>
#include <vector>

namespace toyo {

namespace string {

std::wstring wsubstring(const std::wstring& self, int indexStart, int indexEnd);

std::wstring wslice(const std::wstring& self, int start, int end);

std::wstring wslice(const std::wstring& self, int start);

std::wstring wto_lower_case(const std::wstring& self);

int wlast_index_of(const std::wstring& self, const std::wstring& searchValue, int fromIndex);

int wlast_index_of(const std::wstring& self, const std::wstring& searchValue);

std::vector<std::wstring> wsplit(const std::wstring& self);
std::vector<std::wstring> wsplit(const std::wstring& self, const std::wstring& separator, int limit = -1);

}

}

#endif
