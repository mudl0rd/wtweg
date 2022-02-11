#include "utils.h"
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
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

#ifdef _WIN32
static std::string_view SHLIB_EXTENSION = ".dll";
#else
static std::string_view SHLIB_EXTENSION = ".so";
#endif

unsigned get_filesize(FILE* fp)
	{
		unsigned size = 0;
		unsigned pos = ftell(fp);
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, pos, SEEK_SET);
		return size;
	}

unsigned get_filesize(const char *path)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(fopen(path, "rb"), &fclose);
		if (!input)return 0;
		unsigned size = get_filesize(input.get());
		return size;
	}


inline uint32_t pow2up(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}


	unsigned int crc32(const void* data, unsigned int length)
	{
		static const unsigned int tinf_crc32tab[16] = {
		0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac, 0x76dc4190,
		0x6b6b51f4, 0x4db26158, 0x5005713c, 0xedb88320, 0xf00f9344,
		0xd6d6a3e8, 0xcb61b38c, 0x9b64c2b0, 0x86d3d2d4, 0xa00ae278,
		0xbdbdf21c
		};

		const unsigned char* buf = (const unsigned char*)data;
		unsigned int crc = 0xffffffff;
		unsigned int i;

		if (length == 0) return 0;

		for (i = 0; i < length; ++i)
		{
			crc ^= buf[i];
			crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
			crc = tinf_crc32tab[crc & 0x0f] ^ (crc >> 4);
		}

		return crc ^ 0xffffffff;
	}


void vector_appendbytes(std::vector<uint8_t> &vec ,uint8_t* bytes, size_t len)
	{
		vec.insert(vec.end(), bytes, bytes + len);
	}

std::vector<uint8_t> load_data(const char* path, unsigned * size)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(fopen(path, "rb"), &fclose);
		if (!input)return {};
		unsigned Size = get_filesize(input.get());
		*size = Size;
		std::vector<uint8_t> Memory(Size,0);
		int res = fread((uint8_t*)Memory.data(), 1, Size, input.get());
		return Memory;
	}

bool save_data(unsigned char* data, unsigned size, const char* path)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(fopen(path, "wb"), &fclose);
		if (!input)return false;
		fwrite(data, 1, size, input.get());
		return true;
	}





void* openlib(const char *path)
{
#ifdef _WIN32
  HMODULE handle =LoadLibrary(path);
  if (!handle) { return NULL;}
  return handle;
# else
void *handle = dlopen(path, RTLD_LAZY);
if (!handle) { return NULL; }
return handle;
#endif
}
void* getfunc(void *handle,const char* funcname)
{
#ifdef _WIN32
    return (void*)GetProcAddress((HMODULE)handle,funcname);
#else 
    return dlsym(handle,funcname);
#endif
}
void freelib(void *handle)
{
#ifdef _WIN32
    FreeLibrary((HMODULE)handle);
#else 
    return dlclose(handle);
#endif
}