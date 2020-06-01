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
#include "toyo/util/sha256.h"

#define RIGHTROTATE(x, c) (((x) << (32 - (c))) | ((x) >> (c)))

#define SHA256_FINAL 65

#ifndef SHA256_BUFFER_SIZE
#define SHA256_BUFFER_SIZE (128 * 1024)
#endif

struct sha256_hash {
  uint32_t data[8];
  uint64_t len;
  uint32_t pos;
  uint8_t buf[64];
};

static int error_code = 0;

static const uint32_t k[64] = {
  0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
  0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
  0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
  0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
  0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
  0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
  0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
  0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void to_bytes(uint32_t val, uint8_t *bytes) {
  bytes[3] = (uint8_t) val;
  bytes[2] = (uint8_t) (val >> 8);
  bytes[1] = (uint8_t) (val >> 16);
  bytes[0] = (uint8_t) (val >> 24);
}

static void to_bytes64(uint64_t val, uint8_t *bytes) {
  bytes[7] = (uint8_t) val;
  bytes[6] = (uint8_t) (val >> 8);
  bytes[5] = (uint8_t) (val >> 16);
  bytes[4] = (uint8_t) (val >> 24);
  bytes[3] = (uint8_t) (val >> 32);
  bytes[2] = (uint8_t) (val >> 40);
  bytes[1] = (uint8_t) (val >> 48);
  bytes[0] = (uint8_t) (val >> 56);
}

static uint32_t to_uint32(const uint8_t *bytes) {
  return (uint32_t) bytes[3]
    | ((uint32_t) bytes[2] << 8)
    | ((uint32_t) bytes[1] << 16)
    | ((uint32_t) bytes[0] << 24);
}

#define SHA256_MAIN_LOOP(index) \
  do {\
    s0 = RIGHTROTATE(a, 2) ^ RIGHTROTATE(a, 13) ^ RIGHTROTATE(a, 22);\
    maj = (a & b) ^ (a & c) ^ (b & c);\
    t2 = s0 + maj;\
    s1 = RIGHTROTATE(e, 6) ^ RIGHTROTATE(e, 11) ^ RIGHTROTATE(e, 25);\
    ch = (e & f) ^ ((~e) & g);\
    t1 = h + s1 + ch + k[(index)] + w[(index)];\
    h = g;\
    g = f;\
    f = e;\
    e = d + t1;\
    d = c;\
    c = b;\
    b = a;\
    a = t1 + t2;\
  } while(0);

sha256_hash* sha256_init() {
  sha256_hash* hash = (sha256_hash*)malloc(sizeof(sha256_hash));
  if (hash == NULL) {
    error_code = 1;
    return NULL;
  }

  hash->data[0] = 0x6a09e667;
  hash->data[1] = 0xbb67ae85;
  hash->data[2] = 0x3c6ef372;
  hash->data[3] = 0xa54ff53a;
  hash->data[4] = 0x510e527f;
  hash->data[5] = 0x9b05688c;
  hash->data[6] = 0x1f83d9ab;
  hash->data[7] = 0x5be0cd19;
  
  hash->len = 0;
  hash->pos = 0;
  memset(hash->buf, 0, 64);

  return hash;
}

void sha256_free(sha256_hash* hash) {
  if (hash) {
    free(hash);
  }
}

static void sha256__update(sha256_hash* hash) {
  uint32_t w[64];
  uint32_t a, b, c, d, e, f, g, h;
  uint32_t s0, s1, maj, t2, ch, t1;
  uint32_t i;

  for (i = 0; i < 16; i++)
    w[i] = to_uint32(hash->buf + i * 4);

  for (; i < 64; i++) {
    s0 = (RIGHTROTATE(w[i-15], 7) ^ RIGHTROTATE(w[i-15], 18)) ^ (w[i-15] >> 3);
    s1 = (RIGHTROTATE(w[i-2], 17) ^ RIGHTROTATE(w[i-2], 19)) ^ (w[i-2] >> 10);
    w[i] = w[i-16] + s0 + w[i-7] + s1;
  }
  
  a = hash->data[0];
  b = hash->data[1];
  c = hash->data[2];
  d = hash->data[3];
  e = hash->data[4];
  f = hash->data[5];
  g = hash->data[6];
  h = hash->data[7];

  SHA256_MAIN_LOOP(0)
  SHA256_MAIN_LOOP(1)
  SHA256_MAIN_LOOP(2)
  SHA256_MAIN_LOOP(3)
  SHA256_MAIN_LOOP(4)
  SHA256_MAIN_LOOP(5)
  SHA256_MAIN_LOOP(6)
  SHA256_MAIN_LOOP(7)
  SHA256_MAIN_LOOP(8)
  SHA256_MAIN_LOOP(9)
  SHA256_MAIN_LOOP(10)
  SHA256_MAIN_LOOP(11)
  SHA256_MAIN_LOOP(12)
  SHA256_MAIN_LOOP(13)
  SHA256_MAIN_LOOP(14)
  SHA256_MAIN_LOOP(15)

  SHA256_MAIN_LOOP(16)
  SHA256_MAIN_LOOP(17)
  SHA256_MAIN_LOOP(18)
  SHA256_MAIN_LOOP(19)
  SHA256_MAIN_LOOP(20)
  SHA256_MAIN_LOOP(21)
  SHA256_MAIN_LOOP(22)
  SHA256_MAIN_LOOP(23)
  SHA256_MAIN_LOOP(24)
  SHA256_MAIN_LOOP(25)
  SHA256_MAIN_LOOP(26)
  SHA256_MAIN_LOOP(27)
  SHA256_MAIN_LOOP(28)
  SHA256_MAIN_LOOP(29)
  SHA256_MAIN_LOOP(30)
  SHA256_MAIN_LOOP(31)

  SHA256_MAIN_LOOP(32)
  SHA256_MAIN_LOOP(33)
  SHA256_MAIN_LOOP(34)
  SHA256_MAIN_LOOP(35)
  SHA256_MAIN_LOOP(36)
  SHA256_MAIN_LOOP(37)
  SHA256_MAIN_LOOP(38)
  SHA256_MAIN_LOOP(39)
  SHA256_MAIN_LOOP(40)
  SHA256_MAIN_LOOP(41)
  SHA256_MAIN_LOOP(42)
  SHA256_MAIN_LOOP(43)
  SHA256_MAIN_LOOP(44)
  SHA256_MAIN_LOOP(45)
  SHA256_MAIN_LOOP(46)
  SHA256_MAIN_LOOP(47)

  SHA256_MAIN_LOOP(48)
  SHA256_MAIN_LOOP(49)
  SHA256_MAIN_LOOP(50)
  SHA256_MAIN_LOOP(51)
  SHA256_MAIN_LOOP(52)
  SHA256_MAIN_LOOP(53)
  SHA256_MAIN_LOOP(54)
  SHA256_MAIN_LOOP(55)
  SHA256_MAIN_LOOP(56)
  SHA256_MAIN_LOOP(57)
  SHA256_MAIN_LOOP(58)
  SHA256_MAIN_LOOP(59)
  SHA256_MAIN_LOOP(60)
  SHA256_MAIN_LOOP(61)
  SHA256_MAIN_LOOP(62)
  SHA256_MAIN_LOOP(63)

  hash->data[0] += a;
  hash->data[1] += b;
  hash->data[2] += c;
  hash->data[3] += d;
  hash->data[4] += e;
  hash->data[5] += f;
  hash->data[6] += g;
  hash->data[7] += h;
}

int sha256_update(sha256_hash* hash, const unsigned char* data, int length) {
  int l, left, need, resolve;

  if (hash == NULL || data == NULL || length < 0) {
    error_code = 2;
    return error_code;
  }

  if (hash->pos == SHA256_FINAL) {
    error_code = 3;
    return error_code;
  }

  l = 0;
  do {
    need = 64 - hash->pos;
    left = length - l;
    resolve = need > left ? left : need;
    memcpy(hash->buf + hash->pos, data + l, resolve);
    hash->len += resolve;
    l += resolve;
    if (resolve == need) {
      sha256__update(hash);
      hash->pos = 0;
    } else {
      hash->pos += resolve;
    }
  } while (l < length);

  return 0;
}

int sha256_digest(sha256_hash* hash, char* out) {
  uint8_t hex[32];
  uint32_t i;
  if (hash == NULL || out == NULL) {
    error_code = 2;
    return error_code;
  }
  if (hash->pos == SHA256_FINAL) {
    error_code = 3;
    return error_code;
  }

  hash->buf[hash->pos] = 0x80;
  if (hash->pos < 63) {
    memset(hash->buf + hash->pos + 1, 0, 64 - hash->pos - 1);
  }

  if (hash->pos >= 56) {
    sha256__update(hash);
    memset(hash->buf, 0, 64);
  }

  to_bytes64(hash->len * 8, hash->buf + 56);
  // to_bytes64(hash->len >> 29, hash->buf + 60);
  sha256__update(hash);
  hash->pos = SHA256_FINAL;

  to_bytes(hash->data[0], hex);
  to_bytes(hash->data[1], hex + 4);
  to_bytes(hash->data[2], hex + 8);
  to_bytes(hash->data[3], hex + 12);
  to_bytes(hash->data[4], hex + 16);
  to_bytes(hash->data[5], hex + 20);
  to_bytes(hash->data[6], hex + 24);
  to_bytes(hash->data[7], hex + 28);

  for (i = 0; i < 32; i++) {
    sprintf(out + (i * 2), "%02x", hex[i]);
  }
  out[64] = '\0';
  return 0;
}

int sha256_get_last_error() {
  return error_code;
}

const char* sha256_get_error_message(int code) {
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

void sha256_copy(sha256_hash* dst, sha256_hash* src) {
  if (dst == NULL || src == NULL) {
    return;
  }

  memcpy(dst, src, sizeof(sha256_hash));
}

int sha256_cmp(sha256_hash* l, sha256_hash* r, int* result) {
  int i;
  if (l == NULL || r == NULL) {
    error_code = 2;
    return error_code;
  }

  if (result == NULL) {
    return 0;
  }

  uint8_t hexl[32];
  uint8_t hexr[32];

  to_bytes(l->data[0], hexl);
  to_bytes(l->data[1], hexl + 4);
  to_bytes(l->data[2], hexl + 8);
  to_bytes(l->data[3], hexl + 12);
  to_bytes(l->data[4], hexl + 16);
  to_bytes(l->data[5], hexl + 20);
  to_bytes(l->data[6], hexl + 24);
  to_bytes(l->data[7], hexl + 28);

  to_bytes(r->data[0], hexr);
  to_bytes(r->data[1], hexr + 4);
  to_bytes(r->data[2], hexr + 8);
  to_bytes(r->data[3], hexr + 12);
  to_bytes(r->data[4], hexr + 16);
  to_bytes(r->data[5], hexr + 20);
  to_bytes(r->data[6], hexr + 24);
  to_bytes(r->data[7], hexr + 28);

  for (i = 0; i < 32; i++) {
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

int sha256(const char* message, char* out) {
  int r;
  sha256_hash* hash;
  hash = sha256_init();
  if (hash == NULL) {
    return error_code;
  }

  r = sha256_update(hash, (const unsigned char*)message, (int)strlen(message));
  if (r != 0) goto clean;
  r = sha256_digest(hash, out);
  if (r != 0) goto clean;

clean:
  sha256_free(hash);
  return r;
}

int sha256_file(const char* filepath, char* out) {
  int r;
  FILE* f;
  uint8_t* buf;
  sha256_hash* hash;
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
  
  hash = sha256_init();
  if (hash == NULL) {
    fclose(f);
    return error_code;
  }

  buf = (uint8_t*)malloc(SHA256_BUFFER_SIZE);
  if (buf == NULL) {
    fclose(f);
    sha256_free(hash);
    error_code = 1;
    return error_code;
  }

  while ((read_size = fread(buf, 1, SHA256_BUFFER_SIZE, f)) != 0) {
    sha256_update(hash, buf, (int)read_size);
  }
  free(buf);
  fclose(f);
  r = sha256_digest(hash, out);
  sha256_free(hash);
  return r;
}
