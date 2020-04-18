#ifndef __NODEV_DOWNLOAD_H__
#define __NODEV_DOWNLOAD_H__

#include <string>
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
  double start_time;
  double last_time;
  double end_time;
  std::string path;
  downloadCallback callback;
  void* param;
  long code;
} progressInfo;

bool download (const std::string& url, const std::string& path, downloadCallback callback, void* param);

}

#endif
