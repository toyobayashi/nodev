#include <cstddef>
#include <cstring>

#include "toyo/charset.hpp"
#include "string.hpp"

namespace toyo {

namespace string {

std::wstring wsubstring(const std::wstring& self, int indexStart, int indexEnd) {
  int l = (int)self.length();
  if (l == 0)
    return std::wstring({ self[0], L'\0' });
  if (indexStart >= l) {
    indexStart = l;
  } else if (indexStart < 0) {
    indexStart = 0;
  }

  if (indexEnd >= l) {
    indexEnd = l;
  } else if (indexEnd < 0) {
    indexEnd = 0;
  }

  if (indexStart == indexEnd) return L"";

  if (indexEnd < indexStart) {
    int tmp = indexStart;
    indexStart = indexEnd;
    indexEnd = tmp;
  }

  std::wstring res = L"";

  for (int i = indexStart; i < indexEnd; i++) {
    res += self[i];
  }

  return res;
}

std::wstring wslice(const std::wstring& self, int start, int end) {
  int _length = (int)self.length();
  end--;
  start = start < 0 ? (_length + (start % _length)) : start % _length;
  end = end < 0 ? (_length + (end % _length)) : end % _length;
  if (end < start) {
    int tmp = end;
    end = start;
    start = tmp;
  }

  int len = end - start + 1;

  if (len <= 0) return L"";

  return wsubstring(self, start, end + 1);
}

std::wstring wslice(const std::wstring& self, int start) {
  return wslice(self, start, (int)self.length());
}

std::wstring wto_lower_case(const std::wstring& self) {
  size_t bl = self.length();
  wchar_t* res = new wchar_t[bl + 1];
  memset(res, 0, (bl + 1) * sizeof(wchar_t));
  for (size_t i = 0; i < bl; i++) {
    if (self[i] >= 65 && self[i] <= 90) {
      res[i] = self[i] + 32;
    } else {
      res[i] = self[i];
    }
  }
  std::wstring ret(res);
  delete[] res;
  return ret;
}

int wlast_index_of(const std::wstring& self, const std::wstring& searchValue, int fromIndex) {
  int thisLength = (int)self.length();
  if (fromIndex < 0) {
    fromIndex = 0;
  } else if (fromIndex > thisLength) {
    fromIndex = thisLength;
  }

  int len = (int)searchValue.length();
  for (int i = fromIndex - 1; i >= 0; i--) {
    if (searchValue == wsubstring(self, i, i + len)) {
      return i;
    }
  }
  return -1;
}

int wlast_index_of(const std::wstring& self, const std::wstring& searchValue) {
  return wlast_index_of(self, searchValue, (int)self.length());
}

std::vector<std::wstring> wsplit(const std::wstring& self) {
  return { self };
}

std::vector<std::wstring> wsplit(const std::wstring& self, const std::wstring& separator, int limit) {
  std::wstring copy = self;
  wchar_t* copyBuf = new wchar_t[copy.size() + 1];
  memset(copyBuf, 0, (copy.size() + 1) * sizeof(wchar_t));
  wcscpy(copyBuf, copy.c_str());
#ifdef _MSC_VER
  wchar_t* tokenPtr = _wcstok(copyBuf, separator.c_str());
#else
  wchar_t* buffer;
  wchar_t* tokenPtr = wcstok(copyBuf, separator.c_str(), &buffer);
#endif
  std::vector<std::wstring> res;
  while (tokenPtr != NULL && (limit == -1 ? true : ((int)res.size()) < limit)) {
    res.push_back(tokenPtr);
#ifdef _MSC_VER
    tokenPtr = _wcstok(NULL, separator.c_str());
#else
    tokenPtr = wcstok(NULL, separator.c_str(), &buffer);
#endif
  }
  if (copyBuf[copy.size() - 1] == L'\0') {
    res.push_back(L"");
  }
  delete[] copyBuf;
  return res;
}

}

}
