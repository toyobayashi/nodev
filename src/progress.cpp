#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#else
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#include "progress.hpp"
#include <cstdio>
#include <cmath>
#include "toyo/console.hpp"

namespace nodev {

cli_progress::~cli_progress() {
  printf("\n");
}

cli_progress::cli_progress(const std::string& title, int min, int max, int base, int pos, const std::string& additional):
  _title(title),
  _min(min),
  _max(max),
  _base(base),
  _pos(pos),
  _additional(additional) {}

void cli_progress::set_title(const std::string& title) {
  _title = title;
}

void cli_progress::set_base(int base) {
  _base = base;
}

void cli_progress::set_pos(int pos) {
  _pos = pos;
}

void cli_progress::set_range(int min, int max) {
  _min = min;
  _max = max;
}

void cli_progress::set_additional(const std::string& additional) {
  _additional = additional;
}

void cli_progress::print() {
  // std::size_t outputlen = 0;
  std::size_t terminal_width = toyo::console::get_terminal_width();
  toyo::console::clear_line(0);
  printf("%s ", _title.c_str());
  // outputlen += (_title.length() + 1);

  int progressLength = terminal_width - _title.length() - _additional.length() - 14;
  double p_local = round((double)_base / (double)_max * progressLength);
  double p_current = round((double)_pos / (double)_max * progressLength);
  double percent = (double)(_base + _pos) / (double)_max * 100;
  //printf("\r");
  //printf("%.2lf / %.2lf MB ", (_base + _pos) / 1024 / 1024, _max / 1024 / 1024);
  printf("[");
  // outputlen += 1;
  for (int i = 0; i < (int)p_local; i++) {
    printf("+");
    // outputlen += 1;
  }
  for (int i = 0; i < (int)p_current/* - 1*/; i++) {
    printf("=");
    // outputlen += 1;
  }
  printf(">");
  // outputlen += 1;
  for (int i = 0; i < (int)(progressLength - p_local - p_current); i++) {
    printf(" ");
    // outputlen += 1;
  }
  printf("] ");
  // outputlen += 2;
  printf("%6.2lf%% ", percent);
  // outputlen += 8;
  printf("%s", _additional.c_str());
  fflush(stdout);
  // outputlen += _additional.length();
  // int marginRight = (int)(terminal_width - outputlen);
  // for (int i = 0; i < marginRight - 1; i++) {
  //   printf(" ");
  // }
  // for (int i = 0; i < marginRight - 1; i++) {
  //   printf("\b");
  // }
}

}
