#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#include <stdint.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "toyo/util/md5.h"

#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))

#define MD5_FINAL 65

#ifndef MD5_BUFFER_SIZE
#define MD5_BUFFER_SIZE (128 * 1024)
#endif

struct md5_hash {
  uint32_t data[4];
  uint64_t len;
  uint32_t pos;
  uint8_t buf[64];
};

static int error_code = 0;

static const uint32_t k[64] = {
  0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
  0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
  0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
  0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
  0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
  0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
  0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
  0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
  0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
  0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
  0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
  0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
  0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
  0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
  0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
  0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static const uint32_t r[64] = {
  7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
  5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20, 5,  9, 14, 20,
  4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
  6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
};

static void to_bytes(uint32_t val, uint8_t *bytes) {
  bytes[0] = (uint8_t) val;
  bytes[1] = (uint8_t) (val >> 8);
  bytes[2] = (uint8_t) (val >> 16);
  bytes[3] = (uint8_t) (val >> 24);
}

static void to_bytes64(uint64_t val, uint8_t *bytes) {
  bytes[0] = (uint8_t) val;
  bytes[1] = (uint8_t) (val >> 8);
  bytes[2] = (uint8_t) (val >> 16);
  bytes[3] = (uint8_t) (val >> 24);
  bytes[4] = (uint8_t) (val >> 32);
  bytes[5] = (uint8_t) (val >> 40);
  bytes[6] = (uint8_t) (val >> 48);
  bytes[7] = (uint8_t) (val >> 56);
}

static uint32_t to_uint32(const uint8_t *bytes) {
  return (uint32_t) bytes[0]
    | ((uint32_t) bytes[1] << 8)
    | ((uint32_t) bytes[2] << 16)
    | ((uint32_t) bytes[3] << 24);
}

md5_hash* md5_init() {
  md5_hash* md5 = (md5_hash*)malloc(sizeof(md5_hash));
  if (md5 == NULL) {
    error_code = 1;
    return NULL;
  }
  md5->data[0] = 0x67452301;
  md5->data[1] = 0xefcdab89;
  md5->data[2] = 0x98badcfe;
  md5->data[3] = 0x10325476;
  
  md5->len = 0;
  md5->pos = 0;
  memset(md5->buf, 0, 64);

  return md5;
}

void md5_free(md5_hash* md5) {
  if (md5) {
    free(md5);
  }
}

#define MD5_F(i) f = (b & c) | ((~b) & d); g = (i);
#define MD5_G(i) f = (d & b) | ((~d) & c); g = (5 * (i) + 1) % 16;
#define MD5_H(i) f = b ^ c ^ d; g = (3 * (i) + 5) % 16;
#define MD5_I(i) f = c ^ (b | (~d)); g = (7 * (i)) % 16;
#define MD5_CHANGE(i) temp = d; d = c; c = b; b = b + LEFTROTATE((a + f + k[(i)] + w[g]), r[(i)]); a = temp;
#define MD5_FF(i) do { MD5_F(i) MD5_CHANGE(i) } while(0);
#define MD5_GG(i) do { MD5_G(i) MD5_CHANGE(i) } while(0);
#define MD5_HH(i) do { MD5_H(i) MD5_CHANGE(i) } while(0);
#define MD5_II(i) do { MD5_I(i) MD5_CHANGE(i) } while(0);

static void md5__update(md5_hash* md5) {
  uint32_t w[16];
  uint32_t a, b, c, d, i, f, g, temp;

  for (i = 0; i < 16; i++)
    w[i] = to_uint32(md5->buf + i * 4);
  
  a = md5->data[0];
  b = md5->data[1];
  c = md5->data[2];
  d = md5->data[3];

  MD5_FF(0)
  MD5_FF(1)
  MD5_FF(2)
  MD5_FF(3)
  MD5_FF(4)
  MD5_FF(5)
  MD5_FF(6)
  MD5_FF(7)
  MD5_FF(8)
  MD5_FF(9)
  MD5_FF(10)
  MD5_FF(11)
  MD5_FF(12)
  MD5_FF(13)
  MD5_FF(14)
  MD5_FF(15)

  MD5_GG(16)
  MD5_GG(17)
  MD5_GG(18)
  MD5_GG(19)
  MD5_GG(20)
  MD5_GG(21)
  MD5_GG(22)
  MD5_GG(23)
  MD5_GG(24)
  MD5_GG(25)
  MD5_GG(26)
  MD5_GG(27)
  MD5_GG(28)
  MD5_GG(29)
  MD5_GG(30)
  MD5_GG(31)

  MD5_HH(32)
  MD5_HH(33)
  MD5_HH(34)
  MD5_HH(35)
  MD5_HH(36)
  MD5_HH(37)
  MD5_HH(38)
  MD5_HH(39)
  MD5_HH(40)
  MD5_HH(41)
  MD5_HH(42)
  MD5_HH(43)
  MD5_HH(44)
  MD5_HH(45)
  MD5_HH(46)
  MD5_HH(47)

  MD5_II(48)
  MD5_II(49)
  MD5_II(50)
  MD5_II(51)
  MD5_II(52)
  MD5_II(53)
  MD5_II(54)
  MD5_II(55)
  MD5_II(56)
  MD5_II(57)
  MD5_II(58)
  MD5_II(59)
  MD5_II(60)
  MD5_II(61)
  MD5_II(62)
  MD5_II(63)

  md5->data[0] += a;
  md5->data[1] += b;
  md5->data[2] += c;
  md5->data[3] += d;
}

int md5_update(md5_hash* md5, const unsigned char* data, int length) {
  int l, left, need, resolve;

  if (md5 == NULL || data == NULL || length < 0) {
    error_code = 2;
    return error_code;
  }

  if (md5->pos == MD5_FINAL) {
    error_code = 3;
    return error_code;
  }

  l = 0;
  do {
    need = 64 - md5->pos;
    left = length - l;
    resolve = need > left ? left : need;
    memcpy(md5->buf + md5->pos, data + l, resolve);
    md5->len += resolve;
    l += resolve;
    if (resolve == need) {
      md5__update(md5);
      md5->pos = 0;
    } else {
      md5->pos += resolve;
    }
  } while (l < length);

  return 0;
}

int md5_digest(md5_hash* md5, char* out) {
  uint8_t hex[16];
  uint32_t i;
  if (md5 == NULL || out == NULL) {
    error_code = 2;
    return error_code;
  }
  if (md5->pos == MD5_FINAL) {
    error_code = 3;
    return error_code;
  }

  md5->buf[md5->pos] = 0x80;
  if (md5->pos < 63) {
    memset(md5->buf + md5->pos + 1, 0, 64 - md5->pos - 1);
  }

  if (md5->pos >= 56) {
    md5__update(md5);
    memset(md5->buf, 0, 64);
  }

  to_bytes64(md5->len * 8, md5->buf + 56);
  // to_bytes64(md5->len >> 29, md5->buf + 60);
  md5__update(md5);
  md5->pos = MD5_FINAL;

  to_bytes(md5->data[0], hex);
  to_bytes(md5->data[1], hex + 4);
  to_bytes(md5->data[2], hex + 8);
  to_bytes(md5->data[3], hex + 12);

  for (i = 0; i < 16; i++) {
    sprintf(out + (i * 2), "%02x", hex[i]);
  }
  out[32] = '\0';
  return 0;
}

int md5_get_last_error() {
  return error_code;
}

const char* md5_get_error_message(int code) {
  static char message[256];
  memset(message, 0, 256);
  switch (code) {
    case 0: strcpy(message, "Success."); break;
    case 1: strcpy(message, "Out of memory."); break;
    case 2: strcpy(message, "Invalid argument."); break;
    case 3: strcpy(message, "Digest already called."); break;
    case 4: strcpy(message, "Convert failed."); break;
    case 5: strcpy(message, "Open failed."); break;
    default: strcpy(message, "Unknown error."); break;
  }
  return message;
}

void md5_copy(md5_hash* dst, md5_hash* src) {
  if (dst == NULL || src == NULL) {
    return;
  }

  memcpy(dst, src, sizeof(md5_hash));
}

int md5_cmp(md5_hash* l, md5_hash* r, int* result) {
  int i;
  if (l == NULL || r == NULL) {
    error_code = 2;
    return error_code;
  }

  if (result == NULL) {
    return 0;
  }

  uint8_t hexl[16];
  uint8_t hexr[16];

  to_bytes(l->data[0], hexl);
  to_bytes(l->data[1], hexl + 4);
  to_bytes(l->data[2], hexl + 8);
  to_bytes(l->data[3], hexl + 12);

  to_bytes(r->data[0], hexr);
  to_bytes(r->data[1], hexr + 4);
  to_bytes(r->data[2], hexr + 8);
  to_bytes(r->data[3], hexr + 12);

  for (i = 0; i < 16; i++) {
    if (hexl[i] == hexr[i]) {
      continue;
    }
    if (hexl[i] < hexr[i]) {
      *result = -1;
      return 0;
    }
    if (hexl[i] > hexr[i]) {
      *result = 1;
      return 0;
    }
  }

  *result = 0;
  return 0;
}

int md5(const char* message, char* out) {
  int r;
  md5_hash* hash;
  hash = md5_init();
  if (hash == NULL) {
    return error_code;
  }

  r = md5_update(hash, (const unsigned char*)message, (int)strlen(message));
  if (r != 0) goto clean;
  r = md5_digest(hash, out);
  if (r != 0) goto clean;

clean:
  md5_free(hash);
  return r;
}

int md5_file(const char* filepath, char* out) {
  int r;
  FILE* f;
  uint8_t* buf;
  md5_hash* hash;
  size_t read_size;
#ifdef _WIN32
  unsigned short* wpath;
  r = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, NULL, 0);
  if (r == 0) {
    error_code = 4;
    return error_code;
  }
  wpath = (unsigned short*)malloc(r * sizeof(unsigned short));
  if (wpath == NULL) {
    error_code = 1;
    return error_code;
  }
  r = MultiByteToWideChar(CP_UTF8, 0, filepath, -1, wpath, r);
  if (r == 0) {
    free(wpath);
    error_code = 4;
    return error_code;
  }
  f = _wfopen(wpath, L"rb");
  free(wpath);
#else
  f = fopen(filepath, "rb");
#endif
  if (f == NULL) {
    error_code = 5;
    return error_code;
  }
  
  hash = md5_init();
  if (hash == NULL) {
    fclose(f);
    return error_code;
  }

  buf = (uint8_t*)malloc(MD5_BUFFER_SIZE);
  if (buf == NULL) {
    fclose(f);
    md5_free(hash);
    error_code = 1;
    return error_code;
  }

  while ((read_size = fread(buf, 1, MD5_BUFFER_SIZE, f)) != 0) {
    md5_update(hash, buf, (int)read_size);
  }
  free(buf);
  fclose(f);
  r = md5_digest(hash, out);
  md5_free(hash);
  return r;
}
