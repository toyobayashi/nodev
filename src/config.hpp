#ifndef __NODEV_CONFIG_HPP__
#define __NODEV_CONFIG_HPP__

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <string>
#include <map>
#include "json.hpp"
#include "cli.hpp"
#include "toyo/fs.hpp"
#include "toyo/path.hpp"
#include "toyo/console.hpp"

#ifdef _WIN32
#define NODEV_EOL "\r\n"
#else
#define NODEV_EOL "\n"
#endif

#define JSON_HAS(json, key) ((json).find((key)) != (json).end())
#define JSON_CONFIGURE(json, key, member) \
  do {\
    if (JSON_HAS(json, key) && (json)[(key)].is_string()) {\
      member = (json)[(key)].get<std::string>();\
    }\
  } while (0)

namespace nodev {

class nodev_config {
 public:
  std::string root;
  std::string node_mirror;
  std::string node_cache_dir;
  std::string node_arch;
  std::string npm_mirror;
  std::string npm_cache_dir;
  std::string config_path;

  nodev_config():
    root(toyo::path::__dirname()),
    node_mirror("https://nodejs.org/dist"),
    node_cache_dir("cache/node"),
    node_arch(get_arch()),
    npm_mirror("https://github.com/npm/cli/archive"),
    npm_cache_dir("cache/npm"),
    config_path("") {}
  
  nodev_config(const std::string& config_path): nodev_config() {
    this->config_path = config_path;
  }

  void read_config_file() {
    if (config_path != "" && toyo::fs::exists(config_path)) {
      auto configjson = nlohmann::json::parse(toyo::fs::read_file_to_string(config_path));
      const std::string node_key = "node";
      const std::string root_key = "root";
      const std::string npm_key = "npm";
      const std::string mirror_key = "mirror";
      const std::string cache_dir_key = "cacheDir";
      const std::string arch_key = "arch";
      if (JSON_HAS(configjson, root_key) && configjson[root_key].is_string()) {
        this->root = configjson[root_key].get<std::string>();
      }
      if (JSON_HAS(configjson, node_key) && configjson[node_key].is_object()) {
        nlohmann::json node = configjson[node_key];
        JSON_CONFIGURE(node, mirror_key, node_mirror);
        JSON_CONFIGURE(node, cache_dir_key, node_cache_dir);
        JSON_CONFIGURE(node, arch_key, node_arch);
      }
      if (JSON_HAS(configjson, npm_key) && configjson[node_key].is_object()) {
        nlohmann::json npm = configjson[npm_key];
        JSON_CONFIGURE(npm, mirror_key, npm_mirror);
        JSON_CONFIGURE(npm, cache_dir_key, npm_cache_dir);
      }
    }
  }

  void set_node_mirror(const std::string& value) {
    if (value == "default") {
      node_mirror = "https://nodejs.org/dist";
    } else if (value == "taobao") {
      node_mirror = "https://npm.taobao.org/mirrors/node";
    } else {
      node_mirror = value;
    }

    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["node"]["mirror"] = node_mirror;
      this->write(configjson);
    }
  }

  void set_root(const std::string& value) {
    this->root = value;
    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["root"] = value;
      this->write(configjson);
    }
  }

  void set_node_cache_dir(const std::string& value) {
    node_cache_dir = value;
    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["node"]["cacheDir"] = node_cache_dir;
      this->write(configjson);
    }
  }

  void set_node_arch(const std::string& value) {
    node_arch = value;
    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["node"]["arch"] = node_arch;
      this->write(configjson);
    }
  }

  void set_npm_mirror(const std::string& value) {
    if (value == "default") {
      npm_mirror = "https://github.com/npm/cli/archive";
    } else if (value == "taobao") {
      npm_mirror = "https://npm.taobao.org/mirrors/npm";
    } else {
      npm_mirror = value;
    }

    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["npm"]["mirror"] = npm_mirror;
      this->write(configjson);
    }
  }

  void set_npm_cache_dir(const std::string& value) {
    npm_cache_dir = value;

    if (config_path != "") {
      auto configjson = toyo::fs::exists(config_path) ? nlohmann::json::parse(toyo::fs::read_file_to_string(config_path)) : nlohmann::json();
      configjson["npm"]["cacheDir"] = npm_cache_dir;
      this->write(configjson);
    }
  }

  void write(const nlohmann::json& json) {
    if (config_path != "") {
      toyo::fs::write_file(config_path, json.dump(2) + NODEV_EOL);
    }
  }

  void read_cli(const cli& cli) {
    if (cli.has("node_mirror")) {
      node_mirror = cli.get_option("node_mirror");
    }

    if (cli.has("npm_mirror")) {
      npm_mirror = cli.get_option("npm_mirror");
    }

    if (cli.has("node_arch")) {
      node_arch = cli.get_option("node_arch");
    }
  }

  void print() {
    std::map<std::string, std::string> res;
    res["root"] = this->root;
    res["node_mirror"] = this->node_mirror;
    res["node_cache_dir"] = this->node_cache_dir;
    res["node_arch"] = this->node_arch;
    res["npm_mirror"] = this->npm_mirror;
    res["npm_cache_dir"] = this->npm_cache_dir;

    toyo::console::log(res);
  }
 private:
  std::string get_arch() {
#ifdef _WIN32
  #ifdef _WIN64
    return "x64";
  #else
    int res = 0;
    IsWow64Process(GetCurrentProcess(), &res);
    return res == 0 ? "x86" : "x64";
  #endif
#else
  return "x64";
#endif
  }
};

}

#endif
