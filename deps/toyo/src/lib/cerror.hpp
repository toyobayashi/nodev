#ifndef __CERROR_HPP__
#define __CERROR_HPP__

#include <exception>
#include <string>

class cerror : public std::exception {
public:
  cerror(int code, const std::string& message = "");
  virtual const char* what() const noexcept override;
  int code() const;
private:
  int code_;
  std::string message_;
};

#endif
