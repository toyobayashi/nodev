
#include "program.hpp"
#include "cli.hpp"

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif

  nodev::cli cli(argc, argv);
  nodev::program program(cli);

  std::string command = cli.get_command();
  if (command == "help" || command == "h" || command == "-h") {
    program.help();
    return 0;
  }

  if (command == "version" || command == "v" || command == "-v") {
    program.version();
    return 0;
  }

  if (command == "node_mirror") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.node_mirror();
      return 0;
    }
    program.node_mirror(args[0]);
    return 0;
  }

  if (command == "npm_mirror") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.npm_mirror();
      return 0;
    }
    program.npm_mirror(args[0]);
    return 0;
  }

  if (command == "node_cache") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.node_cache();
      return 0;
    }
    program.node_cache(args[0]);
    return 0;
  }

  if (command == "arch" || command == "node_arch") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.node_arch();
      return 0;
    }
    program.node_arch(args[0]);
    return 0;
  }

  if (command == "npm_cache") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.npm_cache();
      return 0;
    }
    program.npm_cache(args[0]);
    return 0;
  }

  if (command == "root") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      program.root();
      return 0;
    }
    program.root(args[0]);
    return 0;
  }

  if (command == "list" || command == "ls") {
    program.list();
    return 0;
  }

  if (command == "rm" || command == "uninstall") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      toyo::console::log("Example: \n\n  nodev rm 12.16.2");
      return 0;
    }
    program.rm(args[0]);
    return 0;
  }

  if (command == "rm_npm" || command == "rmnpm") {
    return program.rm_npm() ? 0 : 1;
  }

  if (command == "get" || command == "install") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      toyo::console::log("Example: \n\n  nodev get 12.16.2");
      return 0;
    }

    return program.get(args[0]) ? 0 : 1;
  }

  if (command == "get_npm") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      toyo::console::log("Example: \n\n  nodev get_npm 6.14.4");
      return 0;
    }

    return program.get_npm(args[0]) ? 0 : 1;
  }

  if (command == "use") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      toyo::console::log("Example: \n\n  nodev use 12.16.2");
      return 0;
    }

    return program.use(args[0]) ? 0 : 1;
  }

  if (command == "use_npm") {
    auto args = cli.get_argument();
    if (args.size() == 0) {
      toyo::console::log("Example: \n\n  nodev use_npm 6.14.4");
      return 0;
    }

    return program.use_npm(args[0]) ? 0 : 1;
  }

  program.help();
  return 0;
}
