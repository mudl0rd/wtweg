#ifndef utils_h_
#define utils_h_

#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include <io.h>
#include <string>
#include <iostream>
#include <vector>

unsigned get_filesize(const char *path);
void* openlib(const char *path);
void* getfunc(void *handle,const char* funcname);
void freelib(void *handle);
unsigned int crc32(const void* data, unsigned int length);
void vector_appendbytes(std::vector<uint8_t> &vec ,uint8_t* bytes, size_t len);
std::vector<uint8_t> load_data(const char* path, unsigned * size);
std::string base64_decode(const std::string &in);
std::string base64_encode(const std::string &in);
bool save_data(unsigned char* data, unsigned size, const char* path);
uint32_t pow2up(uint32_t v);

#define sizeof_array(arr) sizeof(arr)/sizeof(arr[0])

#endif