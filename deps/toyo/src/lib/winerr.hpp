#ifndef __WINERR_H__
#define __WINERR_H__

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <string>

std::string get_win32_error_message(DWORD code);
std::string get_win32_last_error_message();

#endif
#endif
