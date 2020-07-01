#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#define NODEV_EXE_EXT ".exe"
#define NODEV_PLATFORM "win32"
#else
#define NODEV_EXE_EXT ""
#ifdef __APPLE__
#define NODEV_PLATFORM "darwin"
#else
#define NODEV_PLATFORM "linux"
#endif
#endif

#include "program.hpp"
#include "config.hpp"
#include "progress.hpp"
#include "unzip.hpp"
#include "download.hpp"
#include "toyo/fs.hpp"
#include "toyo/path.hpp"
#include "toyo/console.hpp"
#include "toyo/charset.hpp"
#include "toyo/process.hpp"
#include "toyo/util.hpp"
#include <vector>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <chrono>
#include <thread>
#include <fstream>

#define NODEV_NODE_EXE ("node" NODEV_EXE_EXT)

namespace nodev {

static size_t json_cb (void* buffer, size_t size, size_t nmemb, void* userp) {
  char* tmp = new char[size * nmemb + 1];
  memset(tmp, 0, size * nmemb + 1);
  strcpy(tmp, (char*)buffer);
  *(static_cast<std::string*>(userp)) += tmp;
  delete[] tmp;
  return size * nmemb;
}

std::string program::get_npm_version(const std::string& version) {
  std::string node_version = std::string("v") + version;
  std::string res = "";
  CURL* curl = curl_easy_init();
  struct curl_slist* headers = nullptr;

  headers = curl_slist_append(headers, "Accept: */*");
  headers = curl_slist_append(headers, "User-Agent: Node.js Version Manager");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

  curl_easy_setopt(curl, CURLOPT_URL, "https://nodejs.org/dist/index.json");

  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);

  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, json_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

  CURLcode code = curl_easy_perform(curl);

  if (code != CURLE_OK) {
    printf("%s\n", curl_easy_strerror(code));
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return "0.0.0";
  }

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);

  if (res != "") {
    auto json = nlohmann::json::parse(res);
    unsigned int size = json.size();
    for (unsigned int i = 0; i < size; i++) {
      auto v = json[i]["version"].get<std::string>();
      if (v == node_version) {
        return json[i]["npm"].get<std::string>();
      }
    }
  }

  return "0.0.0";
}

std::string program::get_node_version(const std::string& exe_path) {
  std::string cwd = toyo::process::cwd();
  std::string q = "\"";
  std::string tmpfile = toyo::path::join(cwd, "teststdout.txt");
  std::string tmpfileq = q + tmpfile + q;
  std::string node = "";
  if (exe_path.find(" ") != std::string::npos) {
    node = q + exe_path + q;
  } else {
    node = exe_path;
  }

  std::string cmd = node + " -v>" + tmpfileq;
  int r = ::system(cmd.c_str());
  if (r != 0) {
    // failed
  }
  std::string version = toyo::fs::read_file_to_string(tmpfile);
  toyo::fs::remove(tmpfile);
#ifdef _WIN32
  version = version.substr(1, version.length() - 3);
#else
  version = version.substr(1, version.length() - 2);
#endif
  return version;
}

bool program::is_x64_executable(const std::string& exe_path) {
#ifdef _WIN32
  DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);

  DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;
  HANDLE hFile =
    CreateFileW(toyo::charset::a2w(exe_path).c_str(),
      access_mode,
      share_mode,
      NULL,
      OPEN_ALWAYS,
      FILE_FLAG_SEQUENTIAL_SCAN,
      NULL);

  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }
  DWORD high_size;
  DWORD file_size = GetFileSize(hFile, &high_size);

  DWORD mmf_size = 512 * 1024;
  DWORD size_high = 0;

  HANDLE  hFm = CreateFileMappingW(hFile,
    NULL,
    PAGE_READWRITE,
    size_high,
    mmf_size,
    NULL);

  if (hFm == NULL) {
    CloseHandle(hFile);
    return false;
  }

  size_t view_size = 1024 * 256;
  DWORD view_access = FILE_MAP_ALL_ACCESS;

  char* base_address = (char*)MapViewOfFile(hFm, view_access, 0, 0, view_size);
  if (base_address != NULL) {
    bool flag;
    IMAGE_DOS_HEADER* pDos = (IMAGE_DOS_HEADER*)base_address;
    IMAGE_NT_HEADERS* pNt = (IMAGE_NT_HEADERS*)(pDos->e_lfanew + (char*)pDos);

    if (pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 || pNt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
      flag = true;
    else
      flag = false;
    UnmapViewOfFile(base_address);
    CloseHandle(hFm);
    CloseHandle(hFile);
    return flag;
  }
  else {
    return false;
  }
#else
  return true;
#endif
}

bool program::is_executable(const std::string& exe_path) {
#ifdef _WIN32
  return toyo::path::extname(exe_path) == NODEV_EXE_EXT;
#else
  try {
    toyo::fs::access(exe_path, toyo::fs::x_ok);
    return true;
  } catch (const std::exception&) {
    return false;
  }
#endif
}

std::string program::try_to_absolute(const std::string& p) {
  std::string path = toyo::path::normalize(p);
  if (toyo::path::is_absolute(path)) {
    return path;
  }

  if (path.length() > 0 && path.find("~" + toyo::path::sep) == 0) {
    path = toyo::path::homedir() + path.substr(1);
    return path;
  }

  return path;
}

program::~program() {
  if (config_) {
    delete config_;
    config_ = nullptr;
  }

  if (paths_) {
    delete paths_;
    paths_ = nullptr;
  }
}

program::program() {
  dir = toyo::path::__dirname();
  paths_ = new toyo::path::env_paths(toyo::path::env_paths::create(NODEV_EXECUTABLE_NAME));
  std::string config_file;
  try {
    config_file = toyo::path::join(paths_->config, "nodev.config.json");
  } catch (const std::exception& err) {
    toyo::console::error(err.what());
  }
  config_ = new nodev::nodev_config(config_file);
  try {
    config_->read_config_file();
  } catch (const std::exception& err) {
    toyo::console::error(err.what());
  }
}

program::program(const cli& cli): program() {
  config_->read_cli(cli);
}
std::string program::prefix_() const {
  std::string prefix = try_to_absolute(config_->prefix);
  return toyo::path::is_absolute(prefix) ? prefix : toyo::path::join(dir, prefix);
}
std::string program::root_() const {
#ifdef _WIN32
  return this->prefix_();
#else
  return toyo::path::join(this->prefix_(), "bin");
#endif
}
std::string program::npm_cache_dir() const {
  std::string npm_cache_dir = try_to_absolute(config_->npm_cache_dir);
  return toyo::path::is_absolute(npm_cache_dir) ? npm_cache_dir : toyo::path::join(this->root_(), npm_cache_dir);
}
std::string program::node_cache_dir() const {
  std::string node_cache_dir = try_to_absolute(config_->node_cache_dir);
  return toyo::path::is_absolute(node_cache_dir) ? node_cache_dir : toyo::path::join(this->root_(), node_cache_dir);
}
std::string program::node_name(const std::string& version) const {
  return std::string("node-v") + version + "-" + NODEV_PLATFORM + "-" + config_->node_arch + NODEV_EXE_EXT;
}
std::string program::node_path(const std::string& node_name) const {
  return toyo::path::join(this->node_cache_dir(), node_name);
}
std::string program::global_node_modules_dir() const {
#ifdef _WIN32
  return toyo::path::join(this->root_(), "node_modules");
#else
  return toyo::path::join(this->root_(), "../lib/node_modules");
#endif
}

void program::version() const {
  toyo::console::log(NODEV_VERSION);
}

void program::list() const {
  auto cache_dir = this->node_cache_dir();
  std::string notfound = "Node executable is not found.";
  if (!toyo::fs::exists(cache_dir)) {
    toyo::console::log(notfound);
    return;
  }
  auto li = toyo::fs::readdir(cache_dir);
  std::size_t size = li.size();
  if (size == 0) {
    toyo::console::log(notfound);
    return;
  }
  auto current_exe = toyo::path::join(this->root_(), NODEV_NODE_EXE);
  std::string wver = "";
  if (toyo::fs::exists(current_exe)) {
    wver = get_node_version(current_exe);
  }
  std::string current_arch = is_x64_executable(current_exe) ? "x64" : "x86";
  for (std::size_t i = 0; i < size; i++) {
    auto targetFile = toyo::path::join(cache_dir, li[i]);
    auto ext = toyo::path::extname(targetFile);
    if (!toyo::fs::stat(targetFile).is_directory() && is_executable(targetFile)) {
      std::string nodever = li[i].substr(li[i].find("-v") + 2).substr(0, li[i].substr(li[i].find("-v") + 2).find("-"));
      std::string nodearch = li[i].substr(li[i].find("-x") + 1, 3);
      if (wver == nodever && current_arch == nodearch) {
        printf("  * %s - %s\n", wver.c_str(), current_arch.c_str());
      } else {
        printf("    %s - %s\n", nodever.c_str(), nodearch.c_str());
      }
    }
  }
}

bool program::get(const std::string& version) const {
  std::string node_name = this->node_name(version);
  std::string node_path = this->node_path(node_name);
  std::string node_cache_dir = this->node_cache_dir();
  std::string sha256;
  bool r;
  auto download_callback = [](nodev::progressInfo* info, void* data) {
    cli_progress* prog = (cli_progress*) data;
    if (info->total != -1) {
      prog->set_range(0, info->total);
      prog->set_base(info->size);
      prog->set_pos(info->sum);
      prog->print();
    } else {
      if (info->end) {
        prog->set_range(0, 100);
        prog->set_base(0);
        prog->set_pos(100);
      } else {
        prog->set_range(0, 100);
        prog->set_base(0);
        prog->set_pos(0);
      }
      prog->set_additional(std::to_string(info->sum) + " Byte");
      prog->print();
    }
  };
  bool e = toyo::fs::exists(node_path);
#ifdef _WIN32
  if (!e) {
    cli_progress* progress = new cli_progress(std::string("Downloading ") + node_name, 0, 100, 0, 0);
    std::string url = config_->node_mirror + "/v" + version + "/win-" + config_->node_arch + "/node.exe";
    char msg[256];
    try {
      r = nodev::download(
        url,
        node_path,
        download_callback,
        (void*)progress,
        msg
      );
    } catch (const std::exception& err) {
      toyo::console::error(err.what());
      delete progress;
      return false;
    }
    delete progress;

    if (!r) {
      toyo::console::error(msg);
      return false;
    }
  }

  std::string shasum = toyo::path::join(node_cache_dir, "SHASUMS256-" + version + ".txt");
  if (!toyo::fs::exists(shasum)) {
    cli_progress* progress = new cli_progress(std::string("Downloading SHASUMS256.txt"), 0, 100, 0, 0);
    std::string url = config_->node_mirror + "/v" + version + "/SHASUMS256.txt";
    char msg[256];
    try {
      r = nodev::download(
        url,
        shasum,
        download_callback,
        (void*)progress,
        msg
      );
    } catch (const std::exception& err) {
      toyo::console::error(err.what());
      delete progress;
      return false;
    }

    if (!r) {
      toyo::console::error(msg);
      return false;
    }

    delete progress;
  }

  try {
    sha256 = toyo::util::sha256::calc_file(node_path);
    toyo::console::log("SHA256: " + sha256);
  } catch (const std::exception& err) {
    toyo::console::error(err.what());
    return false;
  }

  std::ifstream shatxtFile(shasum, std::ios::in);
  std::string line = "";

  bool checked = false;
  std::string fname = "win-" + config_->node_arch + "/" + NODEV_NODE_EXE;
  while (std::getline(shatxtFile, line)) {
    auto pos = line.find("  ");
    auto filename = line.substr(pos + 2);
    if (filename == fname) {
      auto hash = line.substr(0, pos);
      if (hash == sha256) {
        checked = true;
        break;
      }
    }
  }

  shatxtFile.close();

  if (e && checked) {
    printf("Version %s (%s) is already installed.\n", version.c_str(), config_->node_arch.c_str());
  }

  return checked;
#else
  std::string tgzname = node_name + ".tar.gz";
  std::string tgzpath = toyo::path::join(node_cache_dir, tgzname);

  if (!e) {
    cli_progress* progress = new cli_progress(std::string("Downloading ") + tgzname, 0, 100, 0, 0);
    std::string url = config_->node_mirror + "/v" + version + "/node-v" + version + "-" + NODEV_PLATFORM + "-" + config_->node_arch + ".tar.gz";
    char msg[256];
    try {
      r = nodev::download(
        url,
        tgzpath,
        download_callback,
        (void*)progress,
        msg
      );
    } catch (const std::exception& err) {
      toyo::console::error(err.what());
      delete progress;
      return false;
    }

    delete progress;

    if (!r) {
      toyo::console::error(msg);
      return false;
    }

    nodev::untgz(tgzpath, toyo::path::join(toyo::path::dirname(tgzpath), "tmp"));
    std::string nodeexe = toyo::path::join(toyo::path::dirname(tgzpath), "tmp", node_name, "bin/node");
    toyo::fs::chmod(nodeexe, 0777);
    toyo::fs::copy_file(nodeexe, node_path);
    toyo::fs::remove(toyo::path::join(toyo::path::dirname(tgzpath), "tmp"));
    toyo::fs::remove(tgzpath);
    return true;
  } else {
    printf("Version %s (%s) is already installed.\n", version.c_str(), config_->node_arch.c_str());
    return true;
  }
#endif
}

bool program::get_npm(const std::string& version) const {
  std::string npm_zip_name = version + ".zip";
  std::string npm_zip_path = toyo::path::join(this->npm_cache_dir(), npm_zip_name);
  bool r;
  auto download_callback = [](nodev::progressInfo* info, void* data) {
    cli_progress* prog = (cli_progress*) data;
    prog->set_range(0, info->total);
    prog->set_base(info->size);
    prog->set_pos(info->sum);
    prog->print();
  };

  if (!toyo::fs::exists(npm_zip_path)) {
    cli_progress* progress = new cli_progress(std::string("Downloading ") + npm_zip_name, 0, 100, 0, 0);
    char msg[256];
    try {
      r = nodev::download(
        config_->npm_mirror + "/v" + version + ".zip",
        npm_zip_path,
        download_callback,
        (void*)progress,
        msg
      );
    } catch (const std::exception& err) {
      toyo::console::error(err.what());
      delete progress;
      return false;
    }

    delete progress;

    if (!r) {
      toyo::console::error(msg);
      return false;
    }

    return true;
  }

  return true;
}

bool program::use(const std::string& version) const {
  std::string node_name = this->node_name(version);
  std::string node_path = this->node_path(node_name);

  if (!toyo::fs::exists(node_path)) {
    if (!this->get(version)) {
      toyo::console::error("Use failed.");
      return false;
    }
  }

  std::string root_dir = this->root_();

  toyo::fs::mkdirs(root_dir);
  try {
    toyo::fs::copy_file(node_path, toyo::path::join(root_dir, "node" NODEV_EXE_EXT));
  } catch (const std::exception& err) {
    toyo::console::error("Use failed.");
    toyo::console::error(std::string("Error: ") + err.what());
    return false;
  }

  if (!toyo::fs::exists(toyo::path::join(global_node_modules_dir(), "npm/package.json"))) {
    std::string npm_ver = get_npm_version(version);
    if (npm_ver != "0.0.0") {
      this->use_npm(npm_ver);
    }
  }

  printf("Now using Node.js %s (%s)\n", version.c_str(), config_->node_arch.c_str());
  return true;
}

bool program::use_npm(const std::string& version) const {
  std::string npm_zip_name = version + ".zip";
  std::string npm_zip_path = toyo::path::join(this->npm_cache_dir(), npm_zip_name);
  std::string root_dir = this->root_();

  if (!toyo::fs::exists(npm_zip_path)) {
    if (!this->get_npm(version)) {
      toyo::console::error("Get npm version failed.");
      return false;
    }
  }
  auto npm_unzip_dir = global_node_modules_dir();
  std::string npmdir = toyo::path::join(npm_unzip_dir, "npm");
  this->rm_npm();

  cli_progress* progress = new cli_progress(std::string("Extracting npm ") + version, 0, 100, 0, 0);
  try {
    nodev::unzip(npm_zip_path, npm_unzip_dir, [](nodev::unzCallbackInfo* info, void* data) {
      cli_progress* prog = (cli_progress*) data;
      prog->set_range(0, info->total);
      prog->set_pos(info->uncompressed);
      prog->print();
    }, progress);
  } catch (const std::exception& err) {
    delete progress;
    toyo::console::error("Use npm failed.");
    toyo::console::error(std::string("Error: ") + err.what());
    return false;
  }

  delete progress;

  toyo::fs::mkdirs(root_dir);
  try {
    toyo::fs::rename(toyo::path::join(npm_unzip_dir, "cli-" + version), npmdir);
    std::string npmbin = toyo::path::join(root_dir, "npm");
    std::string npxbin = toyo::path::join(root_dir, "npx");
#ifdef _WIN32
    toyo::fs::copy(toyo::path::join(npmdir, "bin/npm"), npmbin);
    toyo::fs::copy(toyo::path::join(npmdir, "bin/npm.cmd"), toyo::path::join(root_dir, "npm.cmd"));
    toyo::fs::copy(toyo::path::join(npmdir, "bin/npx"), npxbin);
    toyo::fs::copy(toyo::path::join(npmdir, "bin/npx.cmd"), toyo::path::join(root_dir, "npx.cmd"));
#else
    toyo::fs::symlink(toyo::path::join(npmdir, "bin/npm-cli.js"), npmbin);
    toyo::fs::chmod(npmbin, 0777);
    toyo::fs::symlink(toyo::path::join(npmdir, "bin/npx-cli.js"), npxbin);
    toyo::fs::chmod(npxbin, 0777);
#endif
  } catch (const std::exception& err) {
    toyo::console::error(err.what());
    return false;
  }

  // printf("Now using NPM %s\n", version.c_str());
  return true;
}

bool program::rm_npm() const {
  std::string root_dir = this->root_();
  try {
    std::string npmdir = toyo::path::join(global_node_modules_dir(), "npm");
    toyo::fs::remove(npmdir);
    toyo::fs::remove(toyo::path::join(root_dir, "npm"));
    toyo::fs::remove(toyo::path::join(root_dir, "npx"));
#ifdef _WIN32
    toyo::fs::remove(toyo::path::join(root_dir, "npm.cmd"));
    toyo::fs::remove(toyo::path::join(root_dir, "npx.cmd"));
#endif
  } catch (const std::exception& err) {
    toyo::console::error(err.what());
    return false;
  }
  return true;
}

void program::rm(const std::string& version) const {
  std::string node_name = this->node_name(version);
  std::string node_path = this->node_path(node_name);
  toyo::fs::remove(node_path);
}

void program::node_mirror() const {
  toyo::console::log(config_->node_mirror);
}
void program::node_mirror(const std::string& mirror) {
  config_->set_node_mirror(mirror);
}

void program::prefix() const {
  toyo::console::log(this->prefix_());
}
void program::prefix(const std::string& root) {
  config_->set_prefix(root);
}

void program::npm_mirror() const {
  toyo::console::log(config_->npm_mirror);
}
void program::npm_mirror(const std::string& mirror) {
  config_->set_npm_mirror(mirror);
}

void program::node_arch() const {
  toyo::console::log(config_->node_arch);
}
void program::node_arch(const std::string& arch) {
  config_->set_node_arch(arch);
}

void program::node_cache() const {
  toyo::console::log(this->node_cache_dir());
}
void program::node_cache(const std::string& dir) {
  config_->set_node_cache_dir(dir);
}

void program::npm_cache() const {
  toyo::console::log(this->npm_cache_dir());
}
void program::npm_cache(const std::string& dir) {
  config_->set_npm_cache_dir(dir);
}

void program::help() const {
  toyo::console::log("\nNode.js Version Manager %s\n", NODEV_VERSION);

  toyo::console::log("Usage:\n");
  toyo::console::log("  %s version", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s arch [x86 | x64]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s npm_cache [<npm cache dir>]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s node_cache [<node binary dir>]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s prefix [<node install location dir>]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s list", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s use <node version> [options]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s usenpm <npm version> [options]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s rm <node version>", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s rmnpm", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s get <node version> [options]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s node_mirror [default | taobao | <url>]", NODEV_EXECUTABLE_NAME);
  toyo::console::log("  %s npm_mirror [default | taobao | <url>]\n", NODEV_EXECUTABLE_NAME);

  toyo::console::log("Options:\n");
  toyo::console::log("  --node_arch=<x86 | x64>");
  toyo::console::log("  --node_mirror=<default | taobao | <url>>");
  toyo::console::log("  --npm_mirror=<default | taobao | <url>>\n");

  toyo::console::log("Config file path: " + config_->config_path);
}

}
