#ifndef utils_h_
#define utils_h_

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>

#define irange(i, a, b) \
  int i = (a);          \
  i < (b);              \
  ++i // if you want to use ints all the time
#define range(i, a, b) \
  i = (a);             \
  i < (b);             \
  ++i // if you ever want to use something other than an int
#define zrange(i, b) \
  i = 0;             \
  i < (b);           \
  ++i // if you want to start at zero
#define izrange(i, b) \
  int i = 0;          \
  i < (b);            \
  ++i // if you want to start at zero and use ints

typedef struct
{
  int8_t *_ptr;
  uint32_t _cnt;
  int8_t *_base;
  uint32_t _bufsiz;
  int32_t _eof;
} MEM;

typedef struct
{
  int8_t *_ptr;
  uint32_t _cnt;
  int8_t *_base;
  uint32_t _bufsiz;
  int32_t _eof;
  #ifdef _WIN32
  typedef void* handle;
  /// Windows handle to memory mapping of _file
  void*       _mappedfile;
#else
  typedef int   handle;
#endif
  /// file handle
  handle  _file;
  /// pointer to the file contents mapped into memory
  void*       _mapped;
  uint64_t    _mappedsize;
} MEMMAP;

  enum MEMMAPHint
  {
    Normal,         ///< good overall performance
    SequentialScan, ///< read file only once with few seeks
    RandomAccess    ///< jump around
  };


namespace MudUtil
{
  // misc
  unsigned get_filesize(const char *path);
  void *openlib(const char *path);
  void *getfunc(void *handle, const char *funcname);
  void freelib(void *handle);
  std::string get_wtfwegname();
  const char *get_filename_ext(const char *filename);
  void vector_appendbytes(std::vector<uint8_t> &vec, uint8_t *bytes, size_t len);
  std::vector<uint8_t> load_data(const char *path);
  bool save_data(unsigned char *data, unsigned size, const char *path);
  uint32_t pow2up(uint32_t v);
  int clz(uint32_t x);
  std::string base64_decode(const std::string &in);
  std::string base64_encode(const std::string &in);
  std::string replace_all(std::string str, const std::string &from, const std::string &to);

  MEM *mopen(const int8_t *src, uint32_t length);
  void mclose(MEM *buf);
  int32_t mtell(MEM *buf);
  size_t mread(void *buffer, size_t size, size_t count, MEM *buf);
  size_t mwrite(const void *buffer, size_t size, size_t count, MEM *buf);
  int32_t meof(MEM *buf);
  void mseek(MEM *buf, int32_t offset, int32_t whence);

  MEMMAP *memmap_open(const char* fname, size_t *sz);
  void memmap_close(MEMMAP *buf);
  int32_t memmap_tell(MEMMAP *buf);
  size_t memmap_read(void *buffer, size_t size, size_t count, MEMMAP *buf);
  int32_t memmap_eof(MEMMAP *buf);
  void memmap_seek(MEMMAP *buf, int32_t offset, int32_t whence);

  // compression
  std::vector<unsigned char> compress_deflate(unsigned char *buf, size_t size);
  std::vector<unsigned char> decompress_deflate(
      uint8_t *buf,
      size_t size, size_t uncomp_sz);
  // crypto
  uint32_t crc32(uint32_t initial, const void *data, size_t length);
  uint32_t adler32(uint32_t adler, const uint8_t *data, size_t len);
  void SHA1(uint8_t *hash_data, uint8_t *data, size_t len);
  void MD5(uint8_t *hash_data, uint8_t *data, size_t len);
  void SHA256(uint8_t *hash_data, uint8_t *data, size_t len);
  /*
BYTE hash_bytes[16] = {0};
  int namelen = lstrlen(name);
  MD5((BYTE *)name, namelen, (BYTE *)hash_bytes);

BYTE hash_bytes[20] = {0};
  int namelen = lstrlen(name);
  SHA1((BYTE *)name, namelen, (BYTE *)hash_bytes);
  BYTE hash_bytes[32] = {0};
  int namelen = lstrlen(name);
  SHA256((BYTE *)name, namelen, (BYTE *)hash_bytes);
*/
}

#define sizeof_array(arr) sizeof(arr) / sizeof(arr[0])

#endif