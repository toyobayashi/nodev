#ifndef __TOYO_MD5_H__
#define __TOYO_MD5_H__

#ifdef __cplusplus
extern "C" {
#endif

struct md5_hash;
typedef struct md5_hash md5_hash;

md5_hash* md5_init();
int md5_update(md5_hash*, const unsigned char*, int);
int md5_digest(md5_hash*, char*);
void md5_free(md5_hash*);
int md5_get_last_error();
const char* md5_get_error_message(int);
void md5_copy(md5_hash*, md5_hash*);
int md5_cmp(md5_hash*, md5_hash*, int*);

int md5(const char*, char*);
int md5_file(const char*, char*);

#ifdef __cplusplus
}
#endif

#endif
