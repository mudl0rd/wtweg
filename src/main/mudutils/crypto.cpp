#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <io.h>
#include <vector>
#include <array>
using namespace std;

struct md5
{
  uint32_t h[4];
  uint64_t bytes;
  unsigned char chunk[64];
  long chunk_size;
};

struct sha1
{
  uint32_t h[5];
  uint64_t bytes;
  unsigned char chunk[64];
  long chunk_size;
};

struct sha256
{
  uint32_t h[8];
  uint64_t bytes;
  unsigned char chunk[64];
  long chunk_size;
};

uint32_t bswap32(uint32_t x)
{
  return ((x >> 24) | ((x & 0x00FF0000) >> 8) | ((x & 0x0000FF00) << 8) | (x << 24));
}

uint64_t bswap64(uint64_t a)
{
  return ((a & 0x00000000000000FFULL) << 56) |
         ((a & 0x000000000000FF00ULL) << 40) |
         ((a & 0x0000000000FF0000ULL) << 24) |
         ((a & 0x00000000FF000000ULL) << 8) |
         ((a & 0x000000FF00000000ULL) >> 8) |
         ((a & 0x0000FF0000000000ULL) >> 24) |
         ((a & 0x00FF000000000000ULL) >> 40) |
         ((a & 0xFF00000000000000ULL) >> 56);
}

static const uint32_t SHA256_K[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
static const uint32_t SHA256_H[] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};

static const uint32_t SHA1_H[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
// K[i] = floor(sqrt(N[i]) * pow(2, 30)), N[]={2,3,5,10}
static uint32_t SHA1_K[] = {
    0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6};

static const uint32_t MD5_H[] = {
    0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476};
static const uint32_t MD5_K[] = {
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
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
// shift amount table
static const size_t MD5_S[] = {
    7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
    5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
    4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
    6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
static inline uint32_t rotl(uint32_t v, size_t n)
{
  return (v << n) | (v >> (32 - n));
}
static inline uint32_t rotr(uint32_t v, size_t n)
{
  return (v >> n) | (v << (32 - n));
}
static inline uint32_t md5_f(size_t s, uint32_t b, uint32_t c, uint32_t d)
{
  switch (s)
  {
  case 0:
    return (b & c) | (~b & d);
  case 1:
    return (d & b) | (~d & c);
  case 2:
    return b ^ c ^ d;
  case 3:
    return c ^ (b | ~d);
  default:
    return 0;
  }
}
static inline size_t md5_g(size_t s, size_t i)
{
  switch (s)
  {
  case 0:
    return i;
  case 1:
    return (5 * i + 1) % 16;
  case 2:
    return (3 * i + 5) % 16;
  case 3:
    return (7 * i) % 16;
  default:
    return 0;
  }
}
static void md5_update_(struct md5 *self)
{
  uint32_t *sh = self->h;
  uint32_t *w = (uint32_t *)self->chunk;
  uint32_t a = sh[0], b = sh[1], c = sh[2], d = sh[3];
  size_t i = 0;
  for (size_t s = 0; s < 4; s++)
  {
    for (size_t j = 0; j < 16; j++)
    {
      uint32_t fv = md5_f(s, b, c, d);
      uint32_t t = d;
      d = c;
      c = b;
      b = b + rotl(a + fv + MD5_K[i] + w[md5_g(s, i)], MD5_S[i]);
      a = t;
      i++;
    }
  }
  sh[0] += a;
  sh[1] += b;
  sh[2] += c;
  sh[3] += d;
}
static void md5_tail_(struct md5 *self)
{
  uint64_t bits = self->bytes * 8;
  self->chunk[self->chunk_size] = 0x80;
  memset(&self->chunk[self->chunk_size + 1], 0, 63 - self->chunk_size);
  if (self->chunk_size + 1 + sizeof(bits) > 64)
  {
    md5_update_(self);
    memset(self->chunk, 0, 64 - sizeof(bits));
  }
  memcpy(&self->chunk[64 - sizeof(bits)], &bits, sizeof(bits));
  md5_update_(self);
}

static inline uint32_t sha1_f(size_t s, uint32_t b, uint32_t c, uint32_t d)
{
  switch (s)
  {
  case 0:
    return (b & c) | (~b & d);
  case 1:
  case 3:
    return b ^ c ^ d;
  case 2:
    return (b & c) | (b & d) | (c & d);
  default:
    return 0;
  }
}
static void sha1_update_(struct sha1 *self)
{
  uint32_t *sh = self->h;
  uint32_t *cd = (uint32_t *)self->chunk;
  uint32_t w[80];
  for (size_t i = 0; i < 16; i++)
  {
    w[i] = bswap32(cd[i]);
  }
  for (size_t i = 16; i < 80; i++)
  {
    w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
  }

  uint32_t a = sh[0], b = sh[1], c = sh[2], d = sh[3], e = sh[4];
  uint32_t *pw = w;
  for (size_t s = 0; s < 4; s++)
  {
    for (size_t i = 0; i < 20; i++)
    {
      uint32_t t = rotl(a, 5) + sha1_f(s, b, c, d) + e + SHA1_K[s] + *pw++;
      e = d;
      d = c;
      c = rotl(b, 30);
      b = a;
      a = t;
    }
  }
  sh[0] += a;
  sh[1] += b;
  sh[2] += c;
  sh[3] += d;
  sh[4] += e;
}

static void sha1_tail_(struct sha1 *self)
{
  uint64_t bits = bswap64(self->bytes * 8);
  self->chunk[self->chunk_size] = 0x80;
  memset(&self->chunk[self->chunk_size + 1], 0, 63 - self->chunk_size);
  if (self->chunk_size + 1 + sizeof(bits) > 64)
  {
    sha1_update_(self);
    memset(self->chunk, 0, 64 - sizeof(bits));
  }
  memcpy(&self->chunk[64 - sizeof(bits)], &bits, sizeof(bits));
  sha1_update_(self);
}

static void sha256_update_(struct sha256 *self)
{
  uint32_t *sh = self->h;
  uint32_t *cd = (uint32_t *)self->chunk;
  uint32_t w[64];
  for (size_t i = 0; i < 16; i++)
  {
    w[i] = bswap32(cd[i]);
  }
  for (size_t i = 16; i < 64; i++)
  {
    uint32_t w16 = w[i - 16], w15 = w[i - 15], w7 = w[i - 7], w2 = w[i - 2];
    uint32_t s0 = rotr(w15, 7) ^ rotr(w15, 18) ^ (w15 >> 3);
    uint32_t s1 = rotr(w2, 17) ^ rotr(w2, 19) ^ (w2 >> 10);
    w[i] = w16 + s0 + w7 + s1;
  }
  uint32_t a = sh[0], b = sh[1], c = sh[2], d = sh[3],
           e = sh[4], f = sh[5], g = sh[6], h = sh[7];
  for (size_t i = 0; i < 64; i++)
  {
    uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
    uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
    uint32_t ch = (e & f) ^ (~e & g);
    uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
    uint32_t t1 = h + s1 + ch + SHA256_K[i] + w[i];
    uint32_t t2 = s0 + maj;
    h = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  sh[0] += a;
  sh[1] += b;
  sh[2] += c;
  sh[3] += d;
  sh[4] += e;
  sh[5] += f;
  sh[6] += g;
  sh[7] += h;
}
static void sha256_tail_(struct sha256 *self)
{
  uint64_t bits = bswap64(self->bytes * 8);
  self->chunk[self->chunk_size] = 0x80;
  memset(&self->chunk[self->chunk_size + 1], 0, 63 - self->chunk_size);
  if (self->chunk_size + 1 + sizeof(bits) > 64)
  {
    sha256_update_(self);
    memset(self->chunk, 0, 64 - sizeof(bits));
  }
  memcpy(&self->chunk[64 - sizeof(bits)], &bits, sizeof(bits));
  sha256_update_(self);
}

namespace MudUtil
{

  uint32_t adler32(uint32_t adler, const uint8_t *data, size_t len)
  {
    uint32_t s1 = (adler) ? adler & 0xFFFF : 1 & 0xFFFF;
    uint32_t s2 = (adler) ? adler >> 16 : 1 >> 16;
    for (size_t i = 0; i < len; i++)
    {
      s1 = (s1 + data[i]) % 65521;
      s2 = (s2 + s1) % 65521;
    }
    return s2 << 16 | s1;
  }

  uint32_t crc32(uint32_t initial, const void *data, size_t length)
  {
    static const uint32_t crc32tab[16] = {
        0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190,
        0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344,
        0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
        0xbdbdf21c};

    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = (initial) ? initial ^ 0xffffffff : 0xffffffff;

    if (length == 0)
      return 0;

    for (uint32_t i = 0; i < length; ++i)
    {
      crc ^= buf[i];
      crc = crc32tab[crc & 0x0f] ^ (crc >> 4);
      crc = crc32tab[crc & 0x0f] ^ (crc >> 4);
    }

    return crc ^ 0xffffffff;
  }

  void MD5(uint8_t *hash_data, uint8_t *data, size_t len)
  {
    struct md5 hash;
    memcpy(hash.h, MD5_H, sizeof(MD5_H));
    hash.bytes = 0;
    hash.chunk_size = 0;

    uint8_t *cur = data;
    unsigned char *chunk = hash.chunk;
    size_t chunk_size = hash.chunk_size;
    hash.bytes += len;
    uint8_t *end = &cur[len];
    for (uint8_t *next = &cur[64 - chunk_size]; next <= end; next += 64)
    {
      memcpy(&chunk[chunk_size], cur, next - cur);
      md5_update_(&hash);
      cur = next;
      chunk_size = 0;
    }
    size_t rest = end - cur;
    memcpy(&chunk[chunk_size], cur, rest);
    hash.chunk_size = chunk_size + rest;

    if (hash.chunk_size >= 0)
    {
      md5_tail_(&hash);
      hash.chunk_size = -1;
    }
    memcpy(hash_data, hash.h, sizeof(MD5_H));
  }

  void SHA1(uint8_t *hash_data, uint8_t *data, size_t len)
  {
    struct sha1 hash;
    memcpy(hash.h, SHA1_H, sizeof(SHA1_H));
    hash.bytes = 0;
    hash.chunk_size = 0;

    uint8_t *cur = data;
    unsigned char *chunk = hash.chunk;
    size_t chunk_size = hash.chunk_size;
    hash.bytes += len;
    uint8_t *end = &cur[len];
    for (uint8_t *next = &cur[64 - chunk_size]; next <= end; next += 64)
    {
      memcpy(&chunk[chunk_size], cur, next - cur);
      sha1_update_(&hash);
      cur = next;
      chunk_size = 0;
    }
    size_t rest = end - cur;
    memcpy(&chunk[chunk_size], cur, rest);
    hash.chunk_size = chunk_size + rest;
    if (hash.chunk_size >= 0)
    {
      sha1_tail_(&hash);
      hash.chunk_size = -1;
    }
    uint32_t *buf = (uint32_t *)hash_data;
    for (int i = 0; i < 5; i++)
    {
      buf[i] = bswap32(hash.h[i]);
    }
  }

  void SHA256(uint8_t *hash_data, uint8_t *data, size_t len)
  {
    struct sha256 hash;
    memcpy(hash.h, SHA256_K, sizeof(SHA256_K));
    hash.bytes = 0;
    hash.chunk_size = 0;

    if (hash.chunk_size < 0)
      return;
    uint8_t *cur = data;
    unsigned char *chunk = hash.chunk;
    size_t chunk_size = hash.chunk_size;
    hash.bytes += len;
    uint8_t *end = &cur[len];
    for (uint8_t *next = &cur[64 - chunk_size]; next <= end; next += 64)
    {
      memcpy(&chunk[chunk_size], cur, next - cur);
      sha256_update_(&hash);
      cur = next;
      chunk_size = 0;
    }
    size_t rest = end - cur;
    memcpy(&chunk[chunk_size], cur, rest);
    hash.chunk_size = chunk_size + rest;
    if (hash.chunk_size >= 0)
    {
      sha256_tail_(&hash);
      hash.chunk_size = -1;
    }
    uint32_t *buf = (uint32_t *)hash_data;
    for (int i = 0; i < 8; i++)
    {
      buf[i] = bswap32(hash.h[i]);
    }
  }

}
