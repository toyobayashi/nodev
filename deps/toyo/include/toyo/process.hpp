#ifndef __TOYO_PROCESS_HPP__
#define __TOYO_PROCESS_HPP__

#include <string>
#include <map>

namespace toyo {

namespace process {

std::string cwd();
int pid();
std::string platform();

void* dlopen(const std::string& file, int mode = 1);
int dlclose(void* handle);
void* dlsym(void* handle, const std::string& name);
std::string dlerror();

std::map<std::string, std::string> env();

} // process

} // toyo

#endif
