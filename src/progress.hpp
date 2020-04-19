#ifndef __NODEV_PROGRESS_HPP__
#define __NODEV_PROGRESS_HPP__

#include <string>
#include <cstddef>

namespace nodev {

class cli_progress {
public:
  virtual ~cli_progress();
  cli_progress(const std::string&, int, int, int, int, const std::string& additional = "");
  void set_title(const std::string&);
  void set_base(int);
  void set_pos(int);
  void set_range(int, int);
  void set_additional(const std::string&);
  void print();

private:
  cli_progress();
  std::string _title;
  int _min = 0;
  int _max = 100;
  int _base = 0;
  int _pos = 0;
  std::string _additional;
};

}

#endif
