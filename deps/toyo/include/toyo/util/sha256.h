#ifndef __TOYO_SHA256_H__
#define __TOYO_SHA256_H__

#ifdef __cplusplus
extern "C" {
#endif

struct sha256_hash;
typedef struct sha256_hash sha256_hash;

sha256_hash* sha256_init();
int sha256_update(sha256_hash*, const unsigned char*, int);
int sha256_digest(sha256_hash*, char*);
void sha256_free(sha256_hash*);
int sha256_get_last_error();
const char* sha256_get_error_message(int);
void sha256_copy(sha256_hash*, sha256_hash*);
int sha256_cmp(sha256_hash*, sha256_hash*, int*);

int sha256(const char*, char*);
int sha256_file(const char*, char*);

#ifdef __cplusplus
}
#endif

#endif
