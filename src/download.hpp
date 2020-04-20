#ifndef __NODEV_DOWNLOAD_H__
#define __NODEV_DOWNLOAD_H__

#include <string>
#include <chrono>
#include "curl/curl.h"

namespace nodev {

typedef struct progressInfo progressInfo;

typedef void (*downloadCallback)(progressInfo*, void*);

typedef struct progressInfo {
  CURL* curl;
  FILE* fp;
  long size;
  long sum;
  long total;
  int speed;
  std::chrono::steady_clock::time_point start_time;
  std::chrono::steady_clock::time_point last_time;
  std::chrono::steady_clock::time_point end_time;
  std::string path;
  downloadCallback callback;
  void* param;
  long code;
} progressInfo;

bool download (const std::string& url, const std::string& path, downloadCallback callback, void* param);

}

#endif
