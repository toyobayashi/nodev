#ifndef __NODEV_UNZIP_H__
#define __NODEV_UNZIP_H__

#include <string>

namespace nodev {
  typedef struct unzCallbackInfo {
    std::string name;
    unsigned long currentUncompressed;
    unsigned long currentTotal;
    unsigned long uncompressed;
    unsigned long total;
    unsigned long entry;
  } unzCallbackInfo;

  typedef void (*unzCallback)(unzCallbackInfo*, void*);

  bool unzip(const std::string& zipFilePath, const std::string& outDir, unzCallback callback, void* param);
  bool untgz(const std::string& zipFilePath, const std::string& outDir);
}

#endif
