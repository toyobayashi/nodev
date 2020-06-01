#include <iostream>
#include <cstring>
#include <string>
#include "toyo/charset.hpp"
#include "toyo/process.hpp"
#include "toyo/path.hpp"
#include "toyo/fs.hpp"
#include "toyo/console.hpp"
#include "oid/oid.hpp"
#include "toyo/events.hpp"

#include "cmocha/cmocha.h"

#include "regex.hpp"

using namespace toyo;

static int test_join() {
  expect(path::posix::join("/foo", "bar", "baz/asdf", "quux", "..", "a", "bbb") == "/foo/bar/baz/asdf/a/bbb")
  expect(path::posix::join("中文", "文件夹/1/2", "..") == "中文/文件夹/1")
  return 0;
}

static int test_normalize() {
  expect(path::posix::normalize("/foo/bar//baz/asdf/quux/..") == "/foo/bar/baz/asdf")
  expect(path::win32::normalize("C:////temp\\\\/\\/\\/foo/bar") == "C:\\temp\\foo\\bar")
  return 0;
}

static int test_absolute() {
  expect(path::posix::is_absolute("/foo/bar") == true)
  expect(path::posix::is_absolute("/baz/..") == true)
  expect(path::posix::is_absolute("qux/") == false)
  expect(path::posix::is_absolute(".") == false)

  expect(path::win32::is_absolute("//server") == true)
  expect(path::win32::is_absolute("\\\\server") == true)
  expect(path::win32::is_absolute("C:/foo/..") == true)
  expect(path::win32::is_absolute("C:\\foo\\..") == true)
  expect(path::win32::is_absolute("bar\\baz") == false)
  expect(path::win32::is_absolute("bar/baz") == false)
  expect(path::win32::is_absolute(".") == false)
  return 0;
}

static int test_dirname() {
  expect(path::dirname("/foo/bar/baz/asdf/quux") == "/foo/bar/baz/asdf")
  expect(path::dirname(".") == ".")
  expect(path::dirname("..") == ".")
  return 0;
}

static int test_basename() {
  expect(path::basename("/foo/bar/baz/asdf/quux.html") == "quux.html")
  expect(path::basename("/foo/bar/baz/asdf/quux.html", ".html") == "quux")
  expect(path::basename(".") == ".")
  expect(path::basename("..") == "..")
  return 0;
}

static int test_extname() {
  expect(path::extname("index.html") == ".html")
  expect(path::extname("index.coffee.md") == ".md")
  expect(path::extname("index.") == ".")
  expect(path::extname("index") == "")
  expect(path::extname(".index") == "")
  expect(path::extname(".index.md") == ".md")
  return 0;
}

static int test_resolve() {
#ifdef _WIN32
  expect(path::resolve("/foo/bar", "/tmp/file/").substr(1) == ":\\tmp\\file")
  expect(path::resolve("/foo/bar", "./baz").substr(1) == ":\\foo\\bar\\baz")
#else
  expect(path::resolve("/foo/bar", "/tmp/file/") == "/tmp/file")
  expect(path::resolve("/foo/bar", "./baz") == "/foo/bar/baz")
#endif

  expect(path::resolve("wwwroot", "static_files/png/", "../gif/image.gif") == path::join(toyo::process::cwd(), "wwwroot/static_files/gif/image.gif"))
  return 0;
}

static int test_relative() {
  expect(path::posix::relative("/data/orandea/test/aaa", "/data/orandea/impl/bbb") == "../../impl/bbb")
  expect(path::win32::relative("C:\\orandea\\test\\aaa", "C:\\orandea\\impl\\bbb") == "..\\..\\impl\\bbb")
  return 0;
}

static int test_class() {
  path::path p = path::path::parse_posix("/home/user/dir/file.txt");
  expect(p.root() == "/")
  expect(p.dir() == "/home/user/dir")
  expect(p.base() == "file.txt")
  expect(p.ext() == ".txt")
  expect(p.name() == "file")

  p = path::path::parse_win32("C:\\path\\dir\\file.txt");
  expect(p.root() == "C:\\")
  expect(p.dir() == "C:\\path\\dir")
  expect(p.base() == "file.txt")
  expect(p.ext() == ".txt")
  expect(p.name() == "file")

  path::path obj1;
  obj1
    .root("/ignored")
    .dir("/home/user/dir")
    .base("file.txt");
  expect(obj1.format_posix() == "/home/user/dir/file.txt")

  path::path obj2;
  obj2
    .root("/")
    .base("file.txt")
    .ext("ignored");
  expect(obj2.format_posix() == "/file.txt")

  path::path obj3;
  obj3
    .root("/")
    .name("file")
    .ext(".txt");
  expect(obj3.format_posix() == "/file.txt")

  path::path obj4;
  obj4
    .dir("C:\\path\\dir")
    .name("file.txt");
  expect(obj4.format_win32() == "C:\\path\\dir\\file.txt")

  path::path obj5 = toyo::process::cwd();

  obj5 += "123";

  expect(obj5.format() == path::join(process::cwd(), "123"))

  return 0;
}

static int test_readdir() {
  auto ls = toyo::fs::readdir(path::__dirname() + "/any/..");
  expect(ls.size() > 0)
  console::log(ls);

  fs::mkdirs("emptydir");
  auto ls2 = toyo::fs::readdir("emptydir");
  expect(ls2.size() == 0)
  console::log(ls2);
  fs::remove("emptydir");

  try {
    auto ls2 = toyo::fs::readdir("notexists");
    return -1;
  } catch (const std::exception& e) {
    std::string message = e.what();
    expect(message.find("No such file or directory") != std::string::npos)
    console::error(message);
  }

  return 0;
}

static int test_delimiter_and_sep() {
#ifdef _WIN32
  expect(path::delimiter == ";")
  expect(path::sep == "\\")
#else
  expect(path::delimiter == ":")
  expect(path::sep == "/")
#endif
  return 0;
}

static int test_exists() {
  expect(fs::exists(path::__filename()) == true)
  expect(fs::exists("notexists") == false)
  return 0;
}

static int test_stat() {
  try {
    fs::stats stat = fs::stat("noexists");
    return -1;
  } catch (const std::exception& e) {
    std::string message = e.what();
    expect(message.find("No such file or directory") != std::string::npos)
    console::error(message);
  }

  try {
    fs::stats stat = fs::lstat("noexists");
    return -1;
  } catch (const std::exception& e) {
    std::string message = e.what();
    expect(message.find("No such file or directory") != std::string::npos)
    console::error(message);
  }

  try {
    fs::stats stat = fs::stat(path::__dirname());
    fs::stats stat2 = fs::lstat(path::__dirname());
    expect(stat.is_directory())
    expect(stat2.is_directory())
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  try {
    fs::stats stat = fs::stat(path::__filename());
    fs::stats stat2 = fs::lstat(path::__filename());
    expect(stat.is_file());
    expect(stat2.is_file());
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  try {
    fs::symlink(path::__dirname(), "slk3", fs::symlink_type_junction);
    expect(fs::exists("slk3"))
    fs::remove("slk3");
    expect(!fs::exists("slk3"))
    fs::symlink(path::__filename(), "slk");
    console::log(fs::realpath("slk"));
    expect(fs::exists("slk"))
    fs::stats stat = fs::stat("slk");
    fs::stats stat2 = fs::lstat("slk");
    console::log(stat.size);
    console::log(stat.is_symbolic_link());
    console::log(stat2.size);
    console::log(stat2.is_symbolic_link());
    fs::remove("slk");
    expect(!fs::exists("slk"))
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  try {
    fs::symlink("notexists", "slk2");
    expect(fs::exists("slk2"))
    fs::remove("slk2");
    expect(!fs::exists("slk2"))
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  return 0;
}

static int test_mkdirs() {
  std::string mkdir0 = path::__dirname();
  try {
    fs::mkdirs(mkdir0);
  } catch (const std::exception& e) {
    console::error(e.what());
    return -1;
  }

  std::string mkdir1 = std::string("mkdir_") + process::platform() + "_" + ObjectId().toHexString();
  try {
    fs::mkdirs(mkdir1);
    expect(fs::exists(mkdir1));
    fs::remove(mkdir1);
  } catch (const std::exception& e) {
    console::error(e.what());
    return -1;
  }

  std::string root = std::string("mkdir_") + process::platform() + "_" + ObjectId().toHexString();
  std::string mkdir2 = path::join(root, "subdir/a/b/c");
  try {
    fs::mkdirs(mkdir2);
    expect(fs::exists(mkdir2));
    fs::remove(root);
  } catch (const std::exception& e) {
    console::error(e.what());
    return -1;
  }

  std::string mkdir3 = path::__filename();
  try {
    fs::mkdirs(mkdir3);
    return -1;
  } catch (const std::exception& e) {
    std::string msg = e.what();
    expect(msg.find(strerror(EEXIST)) != std::string::npos)
  }

  std::string mkdir4 = path::join(path::__filename(), "fail");
  try {
    fs::mkdirs(mkdir4);
    return -1;
  } catch (const std::exception& e) {
    std::string msg = e.what();
    expect(msg.find(strerror(ENOENT)) != std::string::npos)
  }

  return 0;
}

static int test_copy() {
  std::string s = "testwrite.txt";
  fs::write_file(s, "666");
  auto d = path::basename(path::__filename() + ".txt");
  try {
    fs::copy_file(s, d);
  } catch (const std::exception& e) {
    fs::remove(s);
    console::error(e.what());
    return 0;
  }
  expect(fs::exists(d))
  try {
    fs::copy_file(s, d, true);
    return -1;
  } catch (const std::exception& e) {
    console::error(e.what());
    fs::remove(d);
    expect(!fs::exists(d))
    fs::remove(s);
    expect(!fs::exists(s))
  }

  try {
    fs::copy("notexist", "any");
    return -1;
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  try {
    fs::mkdirs("./tmp/mkdir/a/b/c");
    fs::write_file("./tmp/mkdir/a/b/c.txt", "1\n");
    fs::write_file("./tmp/mkdir/a/b.txt", "1\n");
    fs::copy("./tmp/mkdir", "./tmp/copy");
    expect(fs::exists("./tmp/copy"))
  } catch (const std::exception& e) {
    console::error(e.what());
    return -1;
  }

  try {
    fs::copy("./tmp/mkdir", "./tmp/copy", true);
    return -1;
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  try {
    fs::copy("./tmp/mkdir", "./tmp/mkdir/subdir");
    return -1;
  } catch (const std::exception& e) {
    console::error(e.what());
  }

  fs::remove("./tmp");

  return 0;
}

static int test_read_write() {
  std::string data = process::platform() + "测试\r\n";
  std::string append = "append";
  try {
    fs::write_file("testwrite.txt", data);
  } catch (const std::exception& e) {
    console::error(e.what());
    return -1;
  }

  expect(fs::read_file_to_string("testwrite.txt") == data)
  fs::append_file("testwrite.txt", append);
  expect(fs::read_file_to_string("testwrite.txt") == (data + append))
  fs::remove("testwrite.txt");

  fs::mkdirs("testmkdir");
  try {
    console::log(fs::read_file_to_string("testmkdir"));
    // std::cout << fs::read_file_to_string("testmkdir") << std::endl;
    return -1;
  } catch (const std::exception&) {
    fs::remove("testmkdir");
  }

  try {
    console::log(fs::read_file_to_string("notexists"));
    // std::cout << fs::read_file_to_string("notexists") << std::endl;
    return -1;
  } catch (const std::exception& err) {
    expect(std::string(err.what()).find("No such file or directory") != std::string::npos)
  }
  return 0;
}

static void test_console() {
  console::log("中文测试");
  console::log(std::vector<std::string>({"中文测试", "2"}));
  console::log(std::vector<unsigned char>({0x01, 0x66}));
  console::log(std::vector<int>({0x01, 0x66}));
  console::log(-9.666);
  console::log(-90);
  console::info("中文测试");
  console::info(std::vector<std::string>({"中文测试", "2"}));
  console::info(std::vector<unsigned char>({0x01, 0x66}));
  console::info(std::vector<int>({0x01, 0x66}));
  console::info(-9.666);
  console::info(-90);
  console::warn("中文测试");
  console::warn(std::vector<std::string>({"中文测试", "2"}));
  console::warn(std::vector<unsigned char>({0x01, 0x66}));
  console::warn(std::vector<int>({0x01, 0x66}));
  console::warn(-9.666);
  console::warn(-90);
  console::error("中文测试");
  console::error(std::vector<std::string>({"中文测试", "2"}));
  console::error(std::vector<unsigned char>({0x01, 0x66}));
  console::error(std::vector<int>({0x01, 0x66}));
  console::error(-9.666);
  console::error(-90);
  console::log(std::string(""));
}

static void test_events() {
  events::event_emitter ev;

  ev
    .on<const std::string&, unsigned int>("newListener", [](const std::string& msg, unsigned int id) {
      console::log("newListener: %s, id: %u", msg.c_str(), id);
    })
    .on<const std::string&, int>("message", [](const std::string& msg, int n) {
      console::log("message: %s, %d", msg.c_str(), n);
    })
    .on<const std::exception&>("error", [](const std::exception& err) {
      console::error(err.what());
    });
  
  ev.emit("error", std::runtime_error("test error1"));

  auto cb = std::function<void(int)>([] (int i) -> void {
    console::log("remove event: %d", i);
    throw std::runtime_error("test error2");
  });
  events::event_id id;
  ev.add_listener<int>("test", cb, &id);
  ev.on<int>("test", [=] (int i) -> void {
    console::log("[=]: %d", i);
  });
  ev.once<int>("test", [&] (int i) -> void {
    console::log("once: %d", i);
  });
  ev.prepend_listener<int>("test", [] (int i) -> void {
    console::log("prepend event: %d", i);
  });
  ev.emit<int>("test", 1);
  ev.off("test", id);
  ev.emit<int>("test", 1);

  ev.emit("message", std::string("test string"), 233);

  console::log(ev.event_names());
  console::log(ev.listener_count("test"));
}

int test() {
  int code = 0;
  int fail = 0;
  code = describe("path",
    test_join,
    test_normalize,
    test_absolute,
    test_dirname,
    test_basename,
    test_extname,
    test_resolve,
    test_relative,
    test_delimiter_and_sep,
    test_class,
    test_globrex);
  
  if (code != 0) {
    fail++;
    code = 0;
  }

  code = describe("fs",
    test_exists,
    test_readdir,
    test_stat,
    test_mkdirs,
    test_copy,
    test_read_write);

  if (code != 0) {
    fail++;
    code = 0;
  }

  int exit_code = fail > 0 ? -1 : 0;

  test_console();
  test_events();

  console::log(process::env());
  console::log(std::string("系统临时目录 tmpdir: ") + path::tmpdir());
  console::log(std::string("家 homedir: ") + path::homedir());

  console::log("%s cwd: %s", charset::a2ocp("当前工作目录").c_str(), toyo::process::cwd().c_str());
  console::log("%s __filename: %s", charset::a2ocp("可执行文件").c_str(), toyo::path::__filename().c_str());
  console::log("%s __dirname: %s", charset::a2ocp("所在目录").c_str(), toyo::path::__dirname().c_str());

  auto env_paths = toyo::path::env_paths::create("toyotest");
  console::log("%s: %s", charset::a2ocp("envPaths.data").c_str(), charset::a2ocp(env_paths.data).c_str());
  console::log("%s: %s", charset::a2ocp("envPaths.config").c_str(), charset::a2ocp(env_paths.config).c_str());
  console::log("%s: %s", charset::a2ocp("envPaths.cache").c_str(), charset::a2ocp(env_paths.cache).c_str());
  console::log("%s: %s", charset::a2ocp("envPaths.log").c_str(), charset::a2ocp(env_paths.log).c_str());
  console::log("%s: %s", charset::a2ocp("envPaths.temp").c_str(), charset::a2ocp(env_paths.temp).c_str());

  console::log(std::string(""));
  console::log("exit: %d, pid: %d", exit_code, toyo::process::pid());
  console::write("erase");
  console::clear_line();
  // console::clear();
  return exit_code;
}
