#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#define MAX_PATH 1024
#endif

#include "unzip.hpp"
#include "contrib/minizip/unzip.h"
// #include "util.h"
#include "toyo/path.hpp"
#include "toyo/fs.hpp"
#include "toyo/charset.hpp"
#include <ctime>
#include <cstring>

int untgz(int argc, char **argv);

#define UNZ_BUFFER_SIZE 128 * 1024

namespace nodev {

static int last = -1;

bool unzip(const std::string& zipFilePath, const std::string& outDir, unzCallback callback, void* param) {
  unzFile unzfile = unzOpen(zipFilePath.c_str());
  if (!unzfile) return false;

  unz_global_info pGlobalInfo;

  int res = unzGetGlobalInfo(unzfile, &pGlobalInfo);
  if (res != UNZ_OK) return false;

  unz_file_info pFileInfo;

  unzCallbackInfo info;
  info.name = "";
  info.currentUncompressed = 0;
  info.currentTotal = 0;
  info.uncompressed = 0;
  info.total = 0;
  info.entry = pGlobalInfo.number_entry;

  for (uLong i = 0; i < pGlobalInfo.number_entry; i++) {
    res = unzGetCurrentFileInfo(unzfile, &pFileInfo, NULL, 0, NULL, 0, NULL, 0);
    if (res != UNZ_OK) return false;

    info.total += pFileInfo.uncompressed_size;

    if ((i + 1) < pGlobalInfo.number_entry) {
      if (UNZ_OK != unzGoToNextFile(unzfile)) return false;
    }
  }

  toyo::fs::mkdirs(outDir);
  if (UNZ_OK != unzGoToFirstFile(unzfile)) return false;

  unsigned char* readBuffer = new unsigned char[UNZ_BUFFER_SIZE];

  char szZipFName[MAX_PATH] = { 0 };
  std::string outfilePath = "";
  std::string tmp = "";

#ifdef _WIN32
  HANDLE hFile = NULL;
#else
  FILE* hFile = NULL;
#endif
  // char* pos = NULL;
  for (uLong i = 0; i < pGlobalInfo.number_entry; i++) {
    memset(szZipFName, 0, sizeof(szZipFName));
    res = unzGetCurrentFileInfo(unzfile, &pFileInfo, szZipFName, MAX_PATH, NULL, 0, NULL, 0);
    if (res != UNZ_OK) {
      delete[] readBuffer;
      return false;
    }

    // pos = szZipFName;
    // while (*pos != 0) {
    //   if (*pos == '/') {
    //     *pos = '\\';
    //   }
    //   pos++;
    // }

    tmp = szZipFName;
    outfilePath = toyo::path::join(outDir, tmp);

    info.name = tmp;

    switch (pFileInfo.external_fa) {
    case 0x00000010 /* FILE_ATTRIBUTE_DIRECTORY */:
      toyo::fs::mkdirs(outfilePath);
      break;
    default:
#ifdef _WIN32
      hFile = CreateFileW(toyo::charset::a2w(outfilePath).c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
      if (hFile == INVALID_HANDLE_VALUE) {
#else
      hFile = fopen(outfilePath.c_str(), "ab+");
      if (hFile == NULL) {
#endif
        toyo::fs::mkdirs(toyo::path::dirname(outfilePath));
#ifdef _WIN32
        hFile = CreateFileW(toyo::charset::a2w(outfilePath).c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_FLAG_WRITE_THROUGH, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
#else
        hFile = fopen(outfilePath.c_str(), "ab+");
        if (hFile == NULL) {
#endif
          delete[] readBuffer;
          return false;
        }
      }

      res = unzOpenCurrentFile(unzfile);
      if (res != UNZ_OK) {
#ifdef _WIN32
        CloseHandle(hFile);
#else
        fclose(hFile);
#endif
        delete[] readBuffer;
        return false;
      }

      info.currentTotal = pFileInfo.uncompressed_size;
      info.currentUncompressed = 0;
      while (1) {
        memset(readBuffer, 0, UNZ_BUFFER_SIZE);
        int nReadFileSize = unzReadCurrentFile(unzfile, readBuffer, UNZ_BUFFER_SIZE);
        if (nReadFileSize < 0) {
          unzCloseCurrentFile(unzfile);
#ifdef _WIN32
          CloseHandle(hFile);
#else
          fclose(hFile);
#endif
          delete[] readBuffer;
          return false;
        }

        if (nReadFileSize == 0) {
          unzCloseCurrentFile(unzfile);
#ifdef _WIN32
          CloseHandle(hFile);
#else
          fclose(hFile);
#endif
          break;
        } else {
          unsigned long dWrite = 0;
#ifdef _WIN32
          int bWriteSuccessed = WriteFile(hFile, readBuffer, nReadFileSize, &dWrite, NULL);
#else
          int bWriteSuccessed = fwrite(readBuffer, 1, nReadFileSize, hFile);
          dWrite = bWriteSuccessed;
          fflush(hFile);
#endif
          info.uncompressed += dWrite;
          info.currentUncompressed += dWrite;
          
          int now = clock();
          if (now - last > 200) {
            // progress(userp->size, userp->sum, userp->total, userp->speed * 2);
            last = now;
            if (callback) {
              callback(&info, param);
            }
          } else if (info.total == info.uncompressed) {
            last = now;
            if (callback) {
              callback(&info, param);
            }
          }

          if (!bWriteSuccessed) {
            unzCloseCurrentFile(unzfile);
#ifdef _WIN32
            CloseHandle(hFile);
#else
            fclose(hFile);
#endif
            delete[] readBuffer;
            return false;
          }
        }
      }

      break;
    }
    if ((i + 1) < pGlobalInfo.number_entry) {
      if (UNZ_OK != unzGoToNextFile(unzfile)) {
        delete[] readBuffer;
        return false;
      }
    }
  }

  if (unzfile) {
    unzCloseCurrentFile(unzfile);
  }

  delete[] readBuffer;
  return true;
}

bool untgz(const std::string& zipFilePath, const std::string& outDir) {
  char argv0[] = "untgz";
  char p[MAX_PATH] = { 0 };
  char outpath[MAX_PATH] = { 0 };
  char out[] = "-o";
  char file[] = "*/bin/node";
  strcpy(p, zipFilePath.c_str());
  strcpy(outpath, outDir.c_str());
  char* argv[5] = { argv0, out, outpath, p, file };
  return ::untgz(5, argv) == 0;
}

}
