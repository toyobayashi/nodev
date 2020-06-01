#ifndef __TOYO_FS_HPP__
#define __TOYO_FS_HPP__

#include <string>
#include <vector>

#include <ctime>

#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32

#else
#include <dirent.h>
#include <unistd.h>
#endif

namespace toyo {

namespace fs {

enum access_type {
  f_ok = 0,
#ifdef _WIN32
  x_ok = 0,
#else
  x_ok = 1,
#endif
  w_ok = 2,
  r_ok = 4,
};

enum symlink_type {
  symlink_type_file,
  symlink_type_directory,
  symlink_type_junction
};

class stats {
private:
  bool is_link_;
public:
  stats();
  stats(const std::string&, bool follow_link = false);
  stats(const char*, bool follow_link = false);

  unsigned int dev;
  unsigned short ino;
  unsigned short mode;
  short nlink;
  short gid;
  short uid;
  unsigned int rdev;
  long size;
  time_t atime;
  time_t mtime;
  time_t ctime;

  bool is_file() const;
  bool is_directory() const;
  bool is_fifo() const;
  bool is_character_device() const;
  bool is_symbolic_link() const;
  bool is_block_device() const;
  bool is_socket() const;
};

#ifdef _WIN32
class dirent;
class dir;

class dirent {
private:
  struct _wfinddata_t* dirent_;

public:
  dirent();
  ~dirent();
  dirent(const dirent& d);
  dirent(dirent&& d);
  dirent(struct _wfinddata_t* d);

  bool is_empty() const;

  const struct _wfinddata_t* data();

  std::string name() const;

  dirent& operator=(const dirent& d);
  dirent& operator=(dirent&& d);

  bool is_file() const;
  bool is_directory() const;
  bool is_fifo() const;
  bool is_character_device() const;
  bool is_symbolic_link() const;
  bool is_block_device() const;
  bool is_socket() const;
};

class dir {
private:
  intptr_t dir_;
  std::string path_;
  struct _wfinddata_t* first_data_;
public:
  dir(const dir&);
  dir(dir&&);
  dir(const std::string& p);
  ~dir();
  void close();
  std::string path() const;
  fs::dirent read();

  dir& operator=(const dir& d);
  dir& operator=(dir&& d);
};
#else
class dirent;
class dir;

class dirent {
private:
  struct ::dirent* dirent_;

public:
  dirent();
  dirent(struct ::dirent* d);

  bool is_empty() const;

  const struct ::dirent* data();

  std::string name() const;

  bool is_file() const;
  bool is_directory() const;
  bool is_fifo() const;
  bool is_character_device() const;
  bool is_symbolic_link() const;
  bool is_block_device() const;
  bool is_socket() const;
};

class dir {
private:
  DIR* dir_;
  std::string path_;
public:
  dir(const std::string& p);
  ~dir();
  void close();
  std::string path() const;
  fs::dirent read() const;
};
#endif

fs::dir opendir(const std::string&);
std::vector<std::string> readdir(const std::string&);
void access(const std::string&, int mode = 0);
void chmod(const std::string&, int mode);
bool exists(const std::string&);
stats stat(const std::string&);
stats lstat(const std::string&);
void mkdir(const std::string&, int mode = 0777);
void mkdirs(const std::string&, int mode = 0777);
void unlink(const std::string&);
void rmdir(const std::string&);
void rename(const std::string&, const std::string&);
void remove(const std::string&);
void symlink(const std::string&, const std::string&);
void symlink(const std::string&, const std::string&, symlink_type);
std::string realpath(const std::string&);
void copy_file(const std::string&, const std::string&, bool fail_if_exists = false);
void copy(const std::string&, const std::string&, bool fail_if_exists = false);
void move(const std::string&, const std::string&);
std::vector<unsigned char> read_file(const std::string&);
std::string read_file_to_string(const std::string&);
void write_file(const std::string&, const std::vector<unsigned char>&);
void write_file(const std::string&, const std::string&);
void append_file(const std::string&, const std::vector<unsigned char>&);
void append_file(const std::string&, const std::string&);

}

}

#endif
