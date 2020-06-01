#ifdef _WIN32
#include "winerr.hpp"
#include <winioctl.h>
#include <bcrypt.h>
#include <direct.h>
#include <io.h>
#include <wchar.h>
#else

#endif

#include <utility>
#include <exception>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <cstdio>
#include <cstddef>

#include "toyo/fs.hpp"
#include "toyo/path.hpp"
#include "toyo/charset.hpp"
#include "cerror.hpp"

#define TOYO_FS_BUFFER_SIZE 128 * 1024

#ifndef _WIN32
#define TOYO__PATH_MAX 8192
#endif

#ifdef _WIN32
static int file_symlink_usermode_flag = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;

static const WCHAR LONG_PATH_PREFIX[] = L"\\\\?\\";
static const WCHAR LONG_PATH_PREFIX_LEN = 4;
static const WCHAR JUNCTION_PREFIX[] = L"\\??\\";
static const WCHAR JUNCTION_PREFIX_LEN = 4;

#define IS_SLASH(c) ((c) == L'\\' || (c) == L'/')
#define IS_LETTER(c) (((c) >= L'a' && (c) <= L'z') || \
  ((c) >= L'A' && (c) <= L'Z'))

#if defined(_MSC_VER) || defined(__MINGW64_VERSION_MAJOR)
  typedef struct _REPARSE_DATA_BUFFER {
    ULONG  ReparseTag;
    USHORT ReparseDataLength;
    USHORT Reserved;
    union {
      struct {
        USHORT SubstituteNameOffset;
        USHORT SubstituteNameLength;
        USHORT PrintNameOffset;
        USHORT PrintNameLength;
        ULONG Flags;
        WCHAR PathBuffer[1];
      } SymbolicLinkReparseBuffer;
      struct {
        USHORT SubstituteNameOffset;
        USHORT SubstituteNameLength;
        USHORT PrintNameOffset;
        USHORT PrintNameLength;
        WCHAR PathBuffer[1];
      } MountPointReparseBuffer;
      struct {
        UCHAR  DataBuffer[1];
      } GenericReparseBuffer;
    };
  } REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;
#endif

typedef struct _FILE_DISPOSITION_INFORMATION {
  BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFORMATION, *PFILE_DISPOSITION_INFORMATION;

typedef struct _IO_STATUS_BLOCK {
  union {
    NTSTATUS Status;
    PVOID Pointer;
  };
  ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _FILE_BASIC_INFORMATION {
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  DWORD FileAttributes;
} FILE_BASIC_INFORMATION, *PFILE_BASIC_INFORMATION;

typedef enum _FILE_INFORMATION_CLASS {
  FileDirectoryInformation = 1,
  FileFullDirectoryInformation,
  FileBothDirectoryInformation,
  FileBasicInformation,
  FileStandardInformation,
  FileInternalInformation,
  FileEaInformation,
  FileAccessInformation,
  FileNameInformation,
  FileRenameInformation,
  FileLinkInformation,
  FileNamesInformation,
  FileDispositionInformation,
  FilePositionInformation,
  FileFullEaInformation,
  FileModeInformation,
  FileAlignmentInformation,
  FileAllInformation,
  FileAllocationInformation,
  FileEndOfFileInformation,
  FileAlternateNameInformation,
  FileStreamInformation,
  FilePipeInformation,
  FilePipeLocalInformation,
  FilePipeRemoteInformation,
  FileMailslotQueryInformation,
  FileMailslotSetInformation,
  FileCompressionInformation,
  FileObjectIdInformation,
  FileCompletionInformation,
  FileMoveClusterInformation,
  FileQuotaInformation,
  FileReparsePointInformation,
  FileNetworkOpenInformation,
  FileAttributeTagInformation,
  FileTrackingInformation,
  FileIdBothDirectoryInformation,
  FileIdFullDirectoryInformation,
  FileValidDataLengthInformation,
  FileShortNameInformation,
  FileIoCompletionNotificationInformation,
  FileIoStatusBlockRangeInformation,
  FileIoPriorityHintInformation,
  FileSfioReserveInformation,
  FileSfioVolumeInformation,
  FileHardLinkInformation,
  FileProcessIdsUsingFileInformation,
  FileNormalizedNameInformation,
  FileNetworkPhysicalNameInformation,
  FileIdGlobalTxDirectoryInformation,
  FileIsRemoteDeviceInformation,
  FileAttributeCacheInformation,
  FileNumaNodeInformation,
  FileStandardLinkInformation,
  FileRemoteProtocolInformation,
  FileMaximumInformation
} FILE_INFORMATION_CLASS, *PFILE_INFORMATION_CLASS;

extern "C"
ULONG
NTAPI
RtlNtStatusToDosError(NTSTATUS Status);

#ifndef NT_SUCCESS
# define NT_SUCCESS(status) (((NTSTATUS) (status)) >= 0)
#endif

extern "C"
NTSYSAPI 
NTSTATUS
NTAPI
NtSetInformationFile(
  IN HANDLE               FileHandle,
  OUT PIO_STATUS_BLOCK    IoStatusBlock,
  IN PVOID                FileInformation,
  IN ULONG                Length,
  IN FILE_INFORMATION_CLASS FileInformationClass );

#endif

#ifdef _WIN32
static int fs_wide_to_utf8(WCHAR* w_source_ptr,
                           DWORD w_source_len,
                           char** target_ptr,
                           uint64_t* target_len_ptr) {
  int r;
  int target_len;
  char* target;
  target_len = WideCharToMultiByte(CP_UTF8,
                                   0,
                                   w_source_ptr,
                                   w_source_len,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);

  if (target_len == 0) {
    return -1;
  }

  if (target_len_ptr != NULL) {
    *target_len_ptr = target_len;
  }

  if (target_ptr == NULL) {
    return 0;
  }

  target = (char*)malloc(target_len + 1);
  if (target == NULL) {
    SetLastError(ERROR_OUTOFMEMORY);
    return -1;
  }

  r = WideCharToMultiByte(CP_UTF8,
                          0,
                          w_source_ptr,
                          w_source_len,
                          target,
                          target_len,
                          NULL,
                          NULL);
  if (r != target_len) {
    return -1;
  }
  target[target_len] = '\0';
  *target_ptr = target;
  return 0;
}

static int fs_readlink_handle(HANDLE handle, char** target_ptr,
    uint64_t* target_len_ptr) {
  char buffer[MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
  REPARSE_DATA_BUFFER* reparse_data = (REPARSE_DATA_BUFFER*) buffer;
  WCHAR* w_target;
  DWORD w_target_len;
  DWORD bytes;

  if (!DeviceIoControl(handle,
                       FSCTL_GET_REPARSE_POINT,
                       NULL,
                       0,
                       buffer,
                       sizeof buffer,
                       &bytes,
                       NULL)) {
    return -1;
  }

  if (reparse_data->ReparseTag == IO_REPARSE_TAG_SYMLINK) {
    /* Real symlink */
    w_target = reparse_data->SymbolicLinkReparseBuffer.PathBuffer +
        (reparse_data->SymbolicLinkReparseBuffer.SubstituteNameOffset /
        sizeof(WCHAR));
    w_target_len =
        reparse_data->SymbolicLinkReparseBuffer.SubstituteNameLength /
        sizeof(WCHAR);

    /* Real symlinks can contain pretty much everything, but the only thing we
     * really care about is undoing the implicit conversion to an NT namespaced
     * path that CreateSymbolicLink will perform on absolute paths. If the path
     * is win32-namespaced then the user must have explicitly made it so, and
     * we better just return the unmodified reparse data. */
    if (w_target_len >= 4 &&
        w_target[0] == L'\\' &&
        w_target[1] == L'?' &&
        w_target[2] == L'?' &&
        w_target[3] == L'\\') {
      /* Starts with \??\ */
      if (w_target_len >= 6 &&
          ((w_target[4] >= L'A' && w_target[4] <= L'Z') ||
           (w_target[4] >= L'a' && w_target[4] <= L'z')) &&
          w_target[5] == L':' &&
          (w_target_len == 6 || w_target[6] == L'\\')) {
        /* \??\<drive>:\ */
        w_target += 4;
        w_target_len -= 4;

      } else if (w_target_len >= 8 &&
                 (w_target[4] == L'U' || w_target[4] == L'u') &&
                 (w_target[5] == L'N' || w_target[5] == L'n') &&
                 (w_target[6] == L'C' || w_target[6] == L'c') &&
                 w_target[7] == L'\\') {
        /* \??\UNC\<server>\<share>\ - make sure the final path looks like
         * \\<server>\<share>\ */
        w_target += 6;
        w_target[0] = L'\\';
        w_target_len -= 6;
      }
    }

  } else if (reparse_data->ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
    /* Junction. */
    w_target = reparse_data->MountPointReparseBuffer.PathBuffer +
        (reparse_data->MountPointReparseBuffer.SubstituteNameOffset /
        sizeof(WCHAR));
    w_target_len = reparse_data->MountPointReparseBuffer.SubstituteNameLength /
        sizeof(WCHAR);

    /* Only treat junctions that look like \??\<drive>:\ as symlink. Junctions
     * can also be used as mount points, like \??\Volume{<guid>}, but that's
     * confusing for programs since they wouldn't be able to actually
     * understand such a path when returned by uv_readlink(). UNC paths are
     * never valid for junctions so we don't care about them. */
    if (!(w_target_len >= 6 &&
          w_target[0] == L'\\' &&
          w_target[1] == L'?' &&
          w_target[2] == L'?' &&
          w_target[3] == L'\\' &&
          ((w_target[4] >= L'A' && w_target[4] <= L'Z') ||
           (w_target[4] >= L'a' && w_target[4] <= L'z')) &&
          w_target[5] == L':' &&
          (w_target_len == 6 || w_target[6] == L'\\'))) {
      SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
      return -1;
    }

    /* Remove leading \??\ */
    w_target += 4;
    w_target_len -= 4;

  } else {
    /* Reparse tag does not indicate a symlink. */
    SetLastError(ERROR_SYMLINK_NOT_SUPPORTED);
    return -1;
  }

  return fs_wide_to_utf8(w_target, w_target_len, target_ptr, target_len_ptr);
}

static void fs_create_junction(const WCHAR* path, const WCHAR* new_path) {
  HANDLE handle = INVALID_HANDLE_VALUE;
  REPARSE_DATA_BUFFER *buffer = NULL;
  int created = 0;
  int target_len;
  int is_absolute, is_long_path;
  int needed_buf_size, used_buf_size, used_data_size, path_buf_len;
  int start, len, i;
  int add_slash;
  DWORD bytes;
  WCHAR* path_buf;

  target_len = (int)wcslen(path);
  is_long_path = wcsncmp(path, LONG_PATH_PREFIX, LONG_PATH_PREFIX_LEN) == 0;

  if (is_long_path) {
    is_absolute = 1;
  } else {
    is_absolute = target_len >= 3 && IS_LETTER(path[0]) &&
      path[1] == L':' && IS_SLASH(path[2]);
  }

  if (!is_absolute) {
    /* Not supporting relative paths */
    throw std::runtime_error("Not supporting relative paths.");
  }

  /* Do a pessimistic calculation of the required buffer size */
  needed_buf_size =
      FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) +
      JUNCTION_PREFIX_LEN * sizeof(WCHAR) +
      2 * (target_len + 2) * sizeof(WCHAR);

  /* Allocate the buffer */
  buffer = (REPARSE_DATA_BUFFER*)malloc(needed_buf_size);
  if (!buffer) {
    throw std::runtime_error("Out of memory.");
  }

  /* Grab a pointer to the part of the buffer where filenames go */
  path_buf = (WCHAR*)&(buffer->MountPointReparseBuffer.PathBuffer);
  path_buf_len = 0;

  /* Copy the substitute (internal) target path */
  start = path_buf_len;

  wcsncpy((WCHAR*)&path_buf[path_buf_len], JUNCTION_PREFIX,
    JUNCTION_PREFIX_LEN);
  path_buf_len += JUNCTION_PREFIX_LEN;

  add_slash = 0;
  for (i = is_long_path ? LONG_PATH_PREFIX_LEN : 0; path[i] != L'\0'; i++) {
    if (IS_SLASH(path[i])) {
      add_slash = 1;
      continue;
    }

    if (add_slash) {
      path_buf[path_buf_len++] = L'\\';
      add_slash = 0;
    }

    path_buf[path_buf_len++] = path[i];
  }
  path_buf[path_buf_len++] = L'\\';
  len = path_buf_len - start;

  /* Set the info about the substitute name */
  buffer->MountPointReparseBuffer.SubstituteNameOffset = (USHORT)(start * sizeof(WCHAR));
  buffer->MountPointReparseBuffer.SubstituteNameLength = (USHORT)(len * sizeof(WCHAR));

  /* Insert null terminator */
  path_buf[path_buf_len++] = L'\0';

  /* Copy the print name of the target path */
  start = path_buf_len;
  add_slash = 0;
  for (i = is_long_path ? LONG_PATH_PREFIX_LEN : 0; path[i] != L'\0'; i++) {
    if (IS_SLASH(path[i])) {
      add_slash = 1;
      continue;
    }

    if (add_slash) {
      path_buf[path_buf_len++] = L'\\';
      add_slash = 0;
    }

    path_buf[path_buf_len++] = path[i];
  }
  len = path_buf_len - start;
  if (len == 2) {
    path_buf[path_buf_len++] = L'\\';
    len++;
  }

  /* Set the info about the print name */
  buffer->MountPointReparseBuffer.PrintNameOffset = (USHORT)(start * sizeof(WCHAR));
  buffer->MountPointReparseBuffer.PrintNameLength = (USHORT)(len * sizeof(WCHAR));

  /* Insert another null terminator */
  path_buf[path_buf_len++] = L'\0';

  /* Calculate how much buffer space was actually used */
  used_buf_size = FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer) +
    path_buf_len * sizeof(WCHAR);
  used_data_size = used_buf_size -
    FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer);

  /* Put general info in the data buffer */
  buffer->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
  buffer->ReparseDataLength = used_data_size;
  buffer->Reserved = 0;

  /* Create a new directory */
  if (!CreateDirectoryW(new_path, NULL)) {
    goto error;
  }
  created = 1;

  /* Open the directory */
  handle = CreateFileW(new_path,
                       GENERIC_WRITE,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_BACKUP_SEMANTICS |
                         FILE_FLAG_OPEN_REPARSE_POINT,
                       NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    goto error;
  }

  /* Create the actual reparse point */
  if (!DeviceIoControl(handle,
                       FSCTL_SET_REPARSE_POINT,
                       buffer,
                       used_buf_size,
                       NULL,
                       0,
                       &bytes,
                       NULL)) {
    goto error;
  }

  /* Clean up */
  CloseHandle(handle);
  free(buffer);

  return;

error:
  free(buffer);

  if (handle != INVALID_HANDLE_VALUE) {
    CloseHandle(handle);
  }

  if (created) {
    RemoveDirectoryW(new_path);
  }

  throw std::runtime_error(get_win32_last_error_message().c_str());
}
#endif

namespace toyo {
namespace fs {

#ifdef _WIN32
dirent::dirent(): dirent_(nullptr) {}
dirent::~dirent() {
  if (dirent_) {
    delete dirent_;
    dirent_ = nullptr;
  }
}
dirent::dirent(struct _wfinddata_t* d): dirent() {
  if (d) {
    dirent_ = new struct _wfinddata_t;
    memcpy(dirent_, d, sizeof(struct _wfinddata_t));
  }
}

dirent::dirent(const dirent& d): dirent(d.dirent_) {}

dirent::dirent(dirent&& d) {
  dirent_ = d.dirent_;
  d.dirent_ = nullptr;
}

bool dirent::is_empty() const { return dirent_ == nullptr; }

const struct _wfinddata_t* dirent::data() { return dirent_; }

std::string dirent::name() const {
  if (dirent_) {
    return toyo::charset::w2a(dirent_->name);
  } else {
    return "";
  }
}

bool dirent::is_file() const {
  if (dirent_) {
    return ((dirent_->attrib & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) && ((dirent_->attrib & FILE_ATTRIBUTE_REPARSE_POINT) != FILE_ATTRIBUTE_REPARSE_POINT);
  } else {
    return false;
  }
}
bool dirent::is_directory() const {
  if (dirent_) {
    return ((dirent_->attrib & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY);
  } else {
    return false;
  }
}
bool dirent::is_fifo() const {
  return false;
}
bool dirent::is_character_device() const {
  return false;
}
bool dirent::is_symbolic_link() const {
  if (dirent_) {
    return ((dirent_->attrib & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT);
  } else {
    return false;
  }
}
bool dirent::is_block_device() const {
  return false;
}
bool dirent::is_socket() const {
  return false;
}

dirent& dirent::operator=(const dirent& d) {
  if (&d == this) return *this;

  if (d.dirent_) {
    if (!dirent_) {
      dirent_ = new struct _wfinddata_t;
    }
    memcpy(dirent_, d.dirent_, sizeof(struct _wfinddata_t));
  } else {
    if (dirent_) {
      delete dirent_;
      dirent_ = nullptr;
    }
  }
  return *this;
}

dirent& dirent::operator=(dirent&& d) {
  if (dirent_) {
    delete dirent_;
    dirent_ = nullptr;
  }
  dirent_ = d.dirent_;
  d.dirent_ = nullptr;
  return *this;
}

dir::dir(const dir& d) {
  dir_ = d.dir_;
  path_ = d.path_;
  if (d.first_data_) {
    first_data_ = new struct _wfinddata_t;
    memcpy(first_data_, d.first_data_, sizeof(struct _wfinddata_t));
  } else {
    first_data_ = nullptr;
  }
}

dir::dir(dir&& d) {
  dir_ = std::exchange(d.dir_, -1);
  path_ = std::move(d.path_);
  first_data_ = d.first_data_;
  d.first_data_ = nullptr;
}

dir& dir::operator=(const dir& d) {
  if (&d == this) return *this;
  dir_ = d.dir_;
  path_ = d.path_;

  if (first_data_) {
    delete first_data_;
    first_data_ = nullptr;
  }

  if (d.first_data_) {
    first_data_ = new struct _wfinddata_t;
    memcpy(first_data_, d.first_data_, sizeof(struct _wfinddata_t));
  } else {
    first_data_ = nullptr;
  }

  return *this;
}

dir& dir::operator=(dir&& d) {
  dir_ = std::exchange(d.dir_, -1);
  path_ = std::move(d.path_);
  if (first_data_) {
    delete first_data_;
    first_data_ = nullptr;
  }
  first_data_ = d.first_data_;
  d.first_data_ = nullptr;
  return *this;
}

dir::dir(const std::string& p): dir_(-1), path_(p), first_data_(nullptr) {
  std::string path = toyo::path::normalize(p);
  std::wstring wpath = toyo::charset::a2w(path);
  first_data_ = new struct _wfinddata_t;
  dir_ = _wfindfirst(toyo::charset::a2w(path::win32::join(path, "*")).c_str(), first_data_);
  if (dir_ == -1) {
    delete first_data_;
    throw cerror(errno, std::string("opendir \"") + p + "\"");
  }
}
dir::~dir() {
  if (first_data_) {
    delete first_data_;
    first_data_ = nullptr;
  }
  if (dir_ != -1) {
    _findclose(dir_);
    dir_ = -1;
  }
}

std::string dir::path() const { return path_; }

void dir::close() {
  if (first_data_) {
    delete first_data_;
    first_data_ = nullptr;
  }
  if (dir_ != -1) {
    if (0 != _findclose(dir_)) {
      throw cerror(errno, std::string("closedir \"") + path_ + "\""); 
    };
    dir_ = -1;
  }
}

fs::dirent dir::read() {
  if (first_data_) {
    fs::dirent tmp(first_data_);
    delete first_data_;
    first_data_ = nullptr;
    return tmp;
  }
  struct _wfinddata_t* file = new struct _wfinddata_t;
  int ret = _wfindnext(dir_, file);
  if (ret == 0) {
    fs::dirent tmp(file);
    delete file;
    return tmp;
  } else {
    delete file;
    return nullptr;
  }
}
#else
dirent::dirent(): dirent_(nullptr) {}
dirent::dirent(struct ::dirent* d): dirent_(d) {}

bool dirent::is_empty() const { return dirent_ == nullptr; }

const struct ::dirent* dirent::data() { return dirent_; }

std::string dirent::name() const {
  if (dirent_) {
    return dirent_->d_name;
  } else {
    return "";
  }
}

bool dirent::is_file() const {
  if (dirent_) {
    return dirent_->d_type == DT_REG;
  } else {
    return false;
  }
}
bool dirent::is_directory() const {
  if (dirent_) {
    return dirent_->d_type == DT_DIR;
  } else {
    return false;
  }
}
bool dirent::is_fifo() const {
  if (dirent_) {
    return dirent_->d_type == DT_FIFO;
  } else {
    return false;
  }
}
bool dirent::is_character_device() const {
  if (dirent_) {
    return dirent_->d_type == DT_FIFO;
  } else {
    return false;
  }
}
bool dirent::is_symbolic_link() const {
  if (dirent_) {
    return dirent_->d_type == DT_LNK;
  } else {
    return false;
  }
}
bool dirent::is_block_device() const {
  if (dirent_) {
    return dirent_->d_type == DT_BLK;
  } else {
    return false;
  }
}
bool dirent::is_socket() const {
  if (dirent_) {
    return dirent_->d_type == DT_SOCK;
  } else {
    return false;
  }
}

dir::dir(const std::string& p): dir_(nullptr), path_(p) {
  std::string path = toyo::path::normalize(p);
  if ((dir_ = ::opendir(path.c_str())) == nullptr) {
    throw cerror(errno, std::string("opendir \"") + p + "\"");
  }
}
dir::~dir() {
  if (dir_) {
    closedir(dir_);
    dir_ = nullptr;
  }
}

std::string dir::path() const { return path_; }

void dir::close() {
  if (dir_) {
    if (0 != closedir(dir_)) {
      throw cerror(errno, std::string("closedir \"") + path_ + "\""); 
    }
    dir_ = nullptr;
  }
}

fs::dirent dir::read() const {
  struct ::dirent *direntp = ::readdir(dir_);
  return direntp;
}

#endif

fs::dir opendir(const std::string& p) {
  return fs::dir(p);
}

stats::stats():
  is_link_(false),
  dev(0),
  ino(0),
  mode(0),
  nlink(0),
  gid(0),
  uid(0),
  rdev(0),
  size(0),
  atime(0),
  mtime(0),
  ctime(0) {}

stats::stats(const char* path, bool follow_link): stats(std::string(path), follow_link) {}

stats::stats(const std::string& p, bool follow_link) {
  std::string path = path::normalize(p);
#ifdef _WIN32
  int code = 0;
  struct _stat info;
  std::wstring wpath = toyo::charset::a2w(path);

  if (follow_link) { // stat
    code = _wstat(wpath.c_str(), &info);
    if (code != 0) {
      throw cerror(errno, std::string("stat") + " \"" + p + "\"");
    }
    this->is_link_ = false;
    this->dev = info.st_dev;
    this->ino = info.st_ino;
    this->mode = info.st_mode;
    this->nlink = info.st_nlink;
    this->gid = info.st_gid;
    this->uid = info.st_uid;
    this->rdev = info.st_rdev;
    this->size = info.st_size;
    this->atime = info.st_atime;
    this->mtime = info.st_mtime;
    this->ctime = info.st_ctime;
    return;
  }

  DWORD attrs = GetFileAttributesW(wpath.c_str());
  if (attrs == INVALID_FILE_ATTRIBUTES) {
    throw cerror(ENOENT, std::string("lstat") + " \"" + p + "\"");
    // win32_throw_stat_error(p, "lstat");
    return;
  }
  bool is_link = ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) == FILE_ATTRIBUTE_REPARSE_POINT);
  if (is_link) {
    this->is_link_ = true;
    this->dev = 0;
    this->ino = 0;
    this->mode = 0;
    this->nlink = 0;
    this->gid = 0;
    this->uid = 0;
    this->rdev = 0;
    this->size = 0;
    this->atime = 0;
    this->mtime = 0;
    this->ctime = 0;
  } else {
    code = _wstat(wpath.c_str(), &info);
    if (code != 0) {
      throw cerror(errno, std::string("lstat") + " \"" + p + "\"");
    }
    this->is_link_ = false;
    this->dev = info.st_dev;
    this->ino = info.st_ino;
    this->mode = info.st_mode;
    this->nlink = info.st_nlink;
    this->gid = info.st_gid;
    this->uid = info.st_uid;
    this->rdev = info.st_rdev;
    this->size = info.st_size;
    this->atime = info.st_atime;
    this->mtime = info.st_mtime;
    this->ctime = info.st_ctime;
  }
#else
  int code = 0;
  struct stat info;
  if (follow_link) {
    code = ::stat(path.c_str(), &info);
    if (code != 0) {
      throw cerror(errno, std::string("stat \"") + p + "\"");
    }
  } else {
    code = ::lstat(path.c_str(), &info);
    if (code != 0) {
      throw cerror(errno, std::string("lstat \"") + p + "\"");
    }
  }

  // this->path_ = path;
  this->is_link_ = S_ISLNK(info.st_mode);
  this->dev = info.st_dev;
  this->ino = info.st_ino;
  this->mode = info.st_mode;
  this->nlink = info.st_nlink;
  this->gid = info.st_gid;
  this->uid = info.st_uid;
  this->rdev = info.st_rdev;
  this->size = info.st_size;
  this->atime = info.st_atime;
  this->mtime = info.st_mtime;
  this->ctime = info.st_ctime;
#endif
}

bool stats::is_file() const {
#ifdef _WIN32
  return ((this->mode & S_IFMT) == S_IFREG);
#else
  return S_ISREG(this->mode);
#endif
}

bool stats::is_directory() const {
#ifdef _WIN32
  return ((this->mode & S_IFMT) == S_IFDIR);
#else
  return S_ISDIR(this->mode);
#endif
}

bool stats::is_fifo() const {
#ifdef _WIN32
  return ((this->mode & S_IFMT) == _S_IFIFO);
#else
  return S_ISFIFO(this->mode);
#endif
}

bool stats::is_character_device() const {
#ifdef _WIN32
  return ((this->mode & S_IFMT) == S_IFCHR);
#else
  return S_ISCHR(this->mode);
#endif
}

bool stats::is_symbolic_link() const {
  return this->is_link_;
}

bool stats::is_block_device() const {
#ifdef _WIN32
  return false;
#else
  return S_ISBLK(this->mode);
#endif
}

bool stats::is_socket() const {
#ifdef _WIN32
  return false;
#else
  return S_ISSOCK(this->mode);
#endif
}

std::vector<std::string> readdir(const std::string& p) {
  std::string newPath = toyo::path::normalize(p);
  fs::dir dir = fs::opendir(newPath);

  std::vector<std::string> res;
  std::string item;

  fs::dirent dirent;

  while (!((dirent = dir.read()).is_empty())) {
    item = dirent.name();
    if (item != "." && item != "..") {
      res.push_back(item);
    }
  }

  dir.close();
  return res;
}

void access(const std::string& p, int mode) {
  std::string npath = path::normalize(p);
#ifdef _WIN32
  std::wstring path = toyo::charset::a2w(npath);

  DWORD attr = GetFileAttributesW(path.c_str());

  if (attr == INVALID_FILE_ATTRIBUTES) {
    // return false;
    throw std::runtime_error((get_win32_last_error_message() + " access \"" + p + "\"").c_str());
  }

  /*
   * Access is possible if
   * - write access wasn't requested,
   * - or the file isn't read-only,
   * - or it's a directory.
   * (Directories cannot be read-only on Windows.)
   */
  if (!(mode & fs::w_ok) ||
      !(attr & FILE_ATTRIBUTE_READONLY) ||
      (attr & FILE_ATTRIBUTE_DIRECTORY)) {
    return;
  } else {
    // return false;
    throw cerror(EPERM, "access \"" + p + "\"");
  }
#else
  if (::access(npath.c_str(), mode) != 0) {
    throw cerror(errno, "access \"" + p + "\"");
  }
#endif
}

void chmod(const std::string& p, int mode) {
  std::string npath = path::normalize(p);
#ifdef _WIN32
  if (::_wchmod(toyo::charset::a2w(npath).c_str(), mode) != 0) {
    throw cerror(errno, "chmod \"" + p + "\"");
  }
#else
  if (::chmod(npath.c_str(), mode) != 0) {
    throw cerror(errno, "chmod \"" + p + "\"");
  }
#endif
}

bool exists(const std::string& p) {
  try {
    fs::access(p, f_ok);
    return true;
  } catch (const std::exception&) {
    try {
      fs::lstat(p);
      return true;
    } catch (const std::exception&) {
      return false;
    }
  }
}

stats stat(const std::string& p) {
  return stats(p, true);
}

stats lstat(const std::string& p) {
  return stats(p, false);
}

void mkdir(const std::string& p, int mode) {
  int code = 0;
  std::string path = path::normalize(p);
#ifdef _WIN32
  code = _wmkdir(toyo::charset::a2w(path).c_str());
#else
  code = ::mkdir(path.c_str(), mode);
#endif
  if (code != 0) {
    throw cerror(errno, "mkdir \"" + p + "\"");
  }
}

void mkdirs(const std::string& p, int mode) {
  if (fs::exists(p)) {
    if (fs::lstat(p).is_directory()) {
      return;
    } else {
      throw cerror(EEXIST, "mkdir \"" + p + "\"");
    }
  }

  std::string dir = path::dirname(p);

  if (!fs::exists(dir)) {
    fs::mkdirs(dir);
  }

  if (fs::lstat(dir).is_directory()) {
    fs::mkdir(p, mode);
  } else {
    throw cerror(ENOENT, "mkdir \"" + p + "\"");
  }
}

void unlink(const std::string& p) {
  int code = 0;
  std::string path = path::normalize(p);
#ifdef _WIN32
  // code = _wunlink(toyo::charset::a2w(path).c_str());

  std::wstring pathws = toyo::charset::a2w(path);
  const WCHAR* pathw = pathws.c_str();
  HANDLE handle;
  BY_HANDLE_FILE_INFORMATION info;
  FILE_DISPOSITION_INFORMATION disposition;
  IO_STATUS_BLOCK iosb;
  NTSTATUS status;

  handle = CreateFileW(pathw,
                       FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES | DELETE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS,
                       NULL);

  if (handle == INVALID_HANDLE_VALUE) {
    throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
  }

  if (!GetFileInformationByHandle(handle, &info)) {
    CloseHandle(handle);
    throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
  }

  if (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
    /* Do not allow deletion of directories, unless it is a symlink. When the
     * path refers to a non-symlink directory, report EPERM as mandated by
     * POSIX.1. */

    /* Check if it is a reparse point. If it's not, it's a normal directory. */
    if (!(info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
      SetLastError(ERROR_ACCESS_DENIED);
      CloseHandle(handle);
      throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
    }

    /* Read the reparse point and check if it is a valid symlink. If not, don't
     * unlink. */
    if (fs_readlink_handle(handle, NULL, NULL) < 0) {
      DWORD error = GetLastError();
      if (error == ERROR_SYMLINK_NOT_SUPPORTED)
        error = ERROR_ACCESS_DENIED;
      SetLastError(error);
      CloseHandle(handle);
      throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
    }
  }

  if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
    /* Remove read-only attribute */
    FILE_BASIC_INFORMATION basic = { 0 };

    basic.FileAttributes = (info.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY) |
                           FILE_ATTRIBUTE_ARCHIVE;

    status = NtSetInformationFile(handle,
                                   &iosb,
                                   &basic,
                                   sizeof basic,
                                   FileBasicInformation);
    if (!NT_SUCCESS(status)) {
      SetLastError(RtlNtStatusToDosError(status));
      CloseHandle(handle);
      throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
    }
  }

  /* Try to set the delete flag. */
  disposition.DeleteFile = TRUE;
  status = NtSetInformationFile(handle,
                                 &iosb,
                                 &disposition,
                                 sizeof disposition,
                                 FileDispositionInformation);
  if (NT_SUCCESS(status)) {
    CloseHandle(handle);
    return;
  } else {
    CloseHandle(handle);
    SetLastError(RtlNtStatusToDosError(status));
    throw std::runtime_error((get_win32_last_error_message() + " unlink \"" + p + "\"").c_str());
  }
#else
  code = ::unlink(path.c_str());
#endif
  if (code != 0) {
    throw cerror(errno, "unlink \"" + p + "\"");
  }
}

void rmdir(const std::string& p) {
  int code = 0;
  std::string path = path::normalize(p);
#ifdef _WIN32
  code = _wrmdir(toyo::charset::a2w(path).c_str());
#else
  code = ::rmdir(path.c_str());
#endif
  if (code != 0) {
    throw cerror(errno, "rmdir \"" + p + "\"");
  }
}

void rename(const std::string& s, const std::string& d) {
  int code = 0;
  std::string source = path::normalize(s);
  std::string dest = path::normalize(d);
#ifdef _WIN32
  code = _wrename(toyo::charset::a2w(source).c_str(), toyo::charset::a2w(dest).c_str());
#else
  code = ::rename(source.c_str(), dest.c_str());
#endif
  if (code != 0) {
    throw cerror(errno, "rename \"" + s + "\" -> \"" + d + "\"");
  }
}

void remove(const std::string& p) {
  if (!fs::exists(p)) {
    return;
  }

  fs::stats stat = fs::lstat(p);
  if (stat.is_directory()) {
    std::vector<std::string> items = fs::readdir(p);
    if (items.size() != 0) {
      for (size_t i = 0; i < items.size(); i++) {
        const std::string& item = items[i];
        fs::remove(path::join(p, item));
      }
      fs::rmdir(p);
    } else {
      fs::rmdir(p);
    }
  } else {
    fs::unlink(p);
  }
}

void symlink(const std::string& o, const std::string& n) {
  std::string oldpath = path::normalize(o);
  std::string newpath = path::normalize(n);
#ifdef _WIN32
  if (!fs::exists(oldpath)) {
    fs::symlink(o, n, symlink_type_file);
  } else if (fs::lstat(oldpath).is_directory()) {
    fs::symlink(o, n, symlink_type_directory);
  } else {
    fs::symlink(o, n, symlink_type_file);
  }
#else
  int code = ::symlink(oldpath.c_str(), newpath.c_str());
  if (code != 0) {
    throw cerror(errno, "symlink \"" + o + "\" -> \"" + n + "\"");
  }
#endif
}

void symlink(const std::string& o, const std::string& n, symlink_type type) {
  std::string oldpath = path::normalize(o);
  std::string newpath = path::normalize(n);
#ifdef _WIN32
  int flags = 0;
  if (type == symlink_type_directory) {
    flags = file_symlink_usermode_flag | SYMBOLIC_LINK_FLAG_DIRECTORY;
    if (!CreateSymbolicLinkW(toyo::charset::a2w(newpath).c_str(), toyo::charset::a2w(oldpath).c_str(), flags)) {
      int err = GetLastError();
      if (err == ERROR_INVALID_PARAMETER && (flags & SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))  {
        file_symlink_usermode_flag = 0;
        fs::symlink(o, n, type);
      } else {
        throw std::runtime_error((get_win32_last_error_message() + " symlink \"" + o + "\" -> \"" + n + "\"").c_str());
      }
    }
  } else if (type == symlink_type_file) {
    flags = file_symlink_usermode_flag;
    if (!CreateSymbolicLinkW(toyo::charset::a2w(newpath).c_str(), toyo::charset::a2w(oldpath).c_str(), flags)) {
      int err = GetLastError();
      if (err == ERROR_INVALID_PARAMETER && (flags & SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))  {
        file_symlink_usermode_flag = 0;
        fs::symlink(o, n, type);
      } else {
        throw std::runtime_error((get_win32_last_error_message() + " symlink \"" + o + "\" -> \"" + n + "\"").c_str());
      }
    }
  } else if (type == symlink_type_junction) {
    oldpath = path::resolve(oldpath);
    fs_create_junction(toyo::charset::a2w(oldpath).c_str(), toyo::charset::a2w(newpath).c_str());
  } else {
    throw std::runtime_error(("Error symlink_type, symlink \"" + o + "\" -> \"" + n + "\"").c_str());
  }
#else
  int code = ::symlink(oldpath.c_str(), newpath.c_str());
  if (code != 0) {
    throw cerror(errno, "symlink \"" + o + "\" -> \"" + n + "\"");
  }
#endif
}

std::string realpath(const std::string& p) {
  std::string path = path::normalize(p);
#ifdef _WIN32
  HANDLE handle;

  handle = CreateFileW(toyo::charset::a2w(path).c_str(),
                       0,
                       0,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
                       NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    throw std::runtime_error(get_win32_last_error_message() + " realpath \"" + p + "\"");
  }

  DWORD w_realpath_len;
  WCHAR* w_realpath_ptr = NULL;
  WCHAR* w_realpath_buf;

  w_realpath_len = GetFinalPathNameByHandleW(handle, NULL, 0, VOLUME_NAME_DOS);
  if (w_realpath_len == 0) {
    throw std::runtime_error(get_win32_last_error_message() + " realpath \"" + p + "\"");
  }

  w_realpath_buf = (WCHAR*)malloc((w_realpath_len + 1) * sizeof(WCHAR));
  if (w_realpath_buf == NULL) {
    SetLastError(ERROR_OUTOFMEMORY);
    throw std::runtime_error(get_win32_last_error_message() + " realpath \"" + p + "\"");
  }
  w_realpath_ptr = w_realpath_buf;

  if (GetFinalPathNameByHandleW(
          handle, w_realpath_ptr, w_realpath_len, VOLUME_NAME_DOS) == 0) {
    free(w_realpath_buf);
    SetLastError(ERROR_INVALID_HANDLE);
    throw std::runtime_error(get_win32_last_error_message() + " realpath \"" + p + "\"");
  }

  /* convert UNC path to long path */
  if (wcsncmp(w_realpath_ptr,
              L"\\\\?\\UNC\\",
              8) == 0) {
    w_realpath_ptr += 6;
    *w_realpath_ptr = L'\\';
    w_realpath_len -= 6;
  } else if (wcsncmp(w_realpath_ptr,
                      LONG_PATH_PREFIX,
                      LONG_PATH_PREFIX_LEN) == 0) {
    w_realpath_ptr += 4;
    w_realpath_len -= 4;
  } else {
    free(w_realpath_buf);
    SetLastError(ERROR_INVALID_HANDLE);
    throw std::runtime_error(get_win32_last_error_message() + " realpath \"" + p + "\"");
  }

  std::string res;
  try {
    res = toyo::charset::w2a(w_realpath_ptr);
  } catch (const std::exception& e) {
    free(w_realpath_buf);
    CloseHandle(handle);
    throw std::runtime_error(std::string(e.what()) + " realpath \"" + p + "\"");
  }

  free(w_realpath_buf);
  return res;
#else
  char* buf = nullptr;

#if defined(_POSIX_VERSION) && _POSIX_VERSION >= 200809L
  buf = ::realpath(path.c_str(), nullptr);
  if (buf == nullptr)
    throw cerror(errno, "realpath \"" + p + "\"");
  return buf;
#else
  ssize_t len;

  ssize_t pathmax;

  pathmax = pathconf(path.c_str(), _PC_PATH_MAX);

  if (pathmax == -1)
    pathmax = TOYO__PATH_MAX;

  len =  pathmax;

  buf = (char*)malloc(len + 1);

  if (buf == nullptr) {
    errno = ENOMEM;
    throw cerror(errno, "realpath \"" + p + "\"");
  }

  if (::realpath(path.c_str(), buf) == NULL) {
    free(buf);
    throw cerror(errno, "realpath \"" + p + "\"");
  }
  std::string res = buf;
  free(buf);
  return res;
#endif
#endif
}

void copy_file(const std::string& s, const std::string& d, bool fail_if_exists) {
  std::string source = path::resolve(s);
  std::string dest = path::resolve(d);
  std::string errmessage = "copy \"" + s + "\" -> \"" + d + "\"";

  if (source == dest) {
    return;
  }

#ifdef _WIN32
  if (!CopyFileW(toyo::charset::a2w(source).c_str(), toyo::charset::a2w(dest).c_str(), fail_if_exists)) {
    throw std::runtime_error((get_win32_last_error_message() + " copy \"" + s + "\" -> \"" + d + "\"").c_str());
  }
#else

  if (fail_if_exists && fs::exists(dest)) {
    throw cerror(EEXIST, errmessage);
  }

  int mode;
  try {
    mode = fs::stat(source).mode;
  } catch (const std::exception&) {
    throw cerror(errno, errmessage);
  }

  FILE* sf = ::fopen(source.c_str(), "rb+");
  if (!sf) {
    throw cerror(errno, errmessage);
  }
  FILE* df = ::fopen(dest.c_str(), "wb+");
  if (!df) {
    ::fclose(sf);
    throw cerror(errno, errmessage);
  }
  unsigned char buf[TOYO_FS_BUFFER_SIZE];
  size_t read;
  while ((read = ::fread(buf, sizeof(unsigned char), TOYO_FS_BUFFER_SIZE, sf)) > 0) {
    ::fwrite(buf, sizeof(unsigned char), read, df);
    ::fflush(df);
  }
  ::fclose(sf);
  ::fclose(df);

  fs::chmod(dest, mode);
#endif

}

void copy(const std::string& s, const std::string& d, bool fail_if_exists) {
  std::string source = path::resolve(s);
  std::string dest = path::resolve(d);

  if (source == dest) {
    return;
  }

  stats stat = fs::lstat(source);

  if (stat.is_directory()) {
    if (path::relative(s, d).find("..") != 0) {
      throw std::runtime_error(std::string("Cannot copy a directory into itself. copy \"") + s + "\" -> \"" + d + "\"");
    }
    fs::mkdirs(dest);
    std::vector<std::string> items = fs::readdir(source);
    for (size_t i = 0; i < items.size(); i++) {
      fs::copy(path::join(source, items[i]), path::join(dest, items[i]), fail_if_exists);
    }
  } else {
    fs::copy_file(source, dest, fail_if_exists);
  }
}

void move(const std::string& s, const std::string& d) {
  std::string source = path::resolve(s);
  std::string dest = path::resolve(d);

  if (source == dest) {
    return;
  }

  fs::copy(source, dest);
  fs::remove(source);
}

std::vector<unsigned char> read_file(const std::string& p) {
  std::string path = path::normalize(p);
  // if (!fs::exists(path)) {
  //   throw cerror(ENOENT, "open \"" + p + "\"");
  // }

  // fs::Stats stat = fs::statSync(path);
  // if (stat.isDirectory()) {
  //   throw std::runtime_error((String("EISDIR: illegal operation on a directory, read \"") + path + "\"").toCString());
  // }

  fs::stats stat;
  try {
    stat = fs::lstat(path);
  } catch (const std::exception&) {
    throw cerror(errno, "open \"" + p + "\"");
  }
  if (stat.is_directory()) {
    throw cerror(EISDIR, "read \"" + p + "\"");
  }

  long size = stat.size;

#ifdef _WIN32
  FILE* fp = ::_wfopen(toyo::charset::a2w(path).c_str(), L"rb");
#else
  FILE* fp = ::fopen(path.c_str(), "rb");
#endif
  if (!fp) {
    throw cerror(errno, "open \"" + p + "\"");
  }

  unsigned char* buf = new unsigned char[size];
  memset(buf, 0, sizeof(unsigned char) * size);
  size_t read = ::fread(buf, sizeof(unsigned char), size, fp);
  ::fclose(fp);
  if (read != size) {
    delete[] buf;
    throw cerror(errno, "read \"" + p + "\"");
  }
  std::vector<unsigned char> res(buf, buf + size);
  delete[] buf;
  return res;
}

std::string read_file_to_string(const std::string& p) {
  std::vector<unsigned char> buf = fs::read_file(p);
  return std::string(buf.begin(), buf.end());
}

static void _write_file(const std::string& p, const std::vector<unsigned char>& buf, std::string mode) {
  if (fs::exists(p)) {
    if (fs::lstat(p).is_directory()) {
      throw cerror(EISDIR, "open \"" + p + "\"");
    }
  }

  std::string path = path::normalize(p);

#ifdef _WIN32
  FILE* fp = ::_wfopen(toyo::charset::a2w(path).c_str(), toyo::charset::a2w(mode).c_str());
#else
  FILE* fp = ::fopen(path.c_str(), mode.c_str());
#endif

  if (!fp) {
    throw cerror(errno, "open \"" + p + "\"");
  }

  ::fwrite(buf.data(), sizeof(unsigned char), buf.size(), fp);
  ::fflush(fp);
  ::fclose(fp);
}

void write_file(const std::string& p, const std::vector<unsigned char>& buf) {
  _write_file(p, buf, "wb");
}

void write_file(const std::string& p, const std::string& str) {
  std::vector<unsigned char> buf(str.begin(), str.end());
  fs::write_file(p, buf);
}

void append_file(const std::string& p, const std::vector<unsigned char>& buf) {
  _write_file(p, buf, "ab");
}

void append_file(const std::string& p, const std::string& str) {
  std::vector<unsigned char> buf(str.begin(), str.end());
  fs::append_file(p, buf);
}

}
}
