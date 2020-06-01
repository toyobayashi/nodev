#ifndef __TOYO_BASE64_H__
#define __TOYO_BASE64_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t base64_encode(const unsigned char* src, size_t len, char* dst);
size_t base64_decode(const char* src, size_t len, unsigned char* dst);

#ifdef __cplusplus
}
#endif

#endif
