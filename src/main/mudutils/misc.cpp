#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#include <stdio.h>
#include "inout.h"
#include <vector>
#include <array>
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#include <SDL2/SDL.h>
#ifdef _WIN32
#include <windows.h>
#include "zip.h"
#include "MemoryModulePP.h"
#else
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#ifdef  _FILE_OFFSET_BITS
#undef  _FILE_OFFSET_BITS
#endif
#define _FILE_OFFSET_BITS 64
// and include needed headers
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#endif
using namespace std;


#ifdef _WIN32
static std::string_view SHLIB_EXTENSION = ".dll";
#else
static std::string_view SHLIB_EXTENSION = ".so";
#endif

namespace MudUtil
{

	std::string replace_all(std::string str, const std::string &from, const std::string &to)
	{
		size_t start_pos = 0;
		while ((start_pos = str.find(from, start_pos)) != std::string::npos)
		{
			str.replace(start_pos, from.length(), to);
			start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}

	unsigned get_filesize(const char *path)
	{
		std::ifstream is(path, std::ifstream::binary);
		if (is)
		{
			// get length of file:
			is.seekg(0, is.end);
			unsigned t = is.tellg();
			is.close();
			return t;
		}
		return 0;
	}

	std::string get_wtfwegname()
	{
#if defined(__linux__) // check defines for your setup
		std::array<char, 1024 * 4> buf{};
		auto written = readlink("/proc/self/exe", buf.data(), buf.size());
		if (written == -1)
			string("");
		return string(buf.data());
#elif defined(_WIN32)
		std::array<char, 1024 * 4> buf{};
		GetModuleFileNameA(nullptr, buf.data(), buf.size());
		return string(buf.data());
#else
		static_assert(false, "unrecognized platform");
#endif
	}

	uint32_t pow2up(uint32_t v)
	{
		return pow(2, ceil(log2(v)));
	}

	int clz(uint32_t x)
	{
		static const char debruijn32[32] = {
			0, 31, 9, 30, 3, 8, 13, 29, 2, 5, 7, 21, 12, 24, 28, 19,
			1, 10, 4, 14, 6, 22, 25, 20, 11, 15, 23, 26, 16, 27, 17, 18};
		return debruijn32[((x & -x) * 0x077CB531) >> 27];
	}

	void vector_appendbytes(std::vector<uint8_t> &vec, uint8_t *bytes, size_t len)
	{
		vec.insert(vec.end(), bytes, bytes + len);
	}

	std::vector<uint8_t> load_data(const char *path)
	{
		std::ifstream input(path, std::ifstream::binary);
		if (!input)
			return {};
		input.seekg(0, input.end);
		unsigned Size = input.tellg();
		input.seekg(0, input.beg);
		std::vector<uint8_t> Memory(Size, 0);
		input.read((char *)&Memory[0], Size);
		input.close();
		return Memory;
	}

	bool save_data(unsigned char *data, unsigned size, const char *path)
	{
		std::ofstream input(path, std::ofstream::binary | std::ios::trunc);
		if (!input.good())
			return false;
		input.write((char *)data, size);
		input.close();
		return true;
	}

	const char *get_filename_ext(const char *filename)
	{
		const char *dot = strrchr(filename, '.');
		if (!dot || dot == filename)
			return "";
		return dot + 1;
	}

	void *openlib(const char *path)
	{
#ifdef _WIN32
#ifndef DEBUG

		if (strcmp(get_filename_ext(path), "zip") == 0)
		{
			struct zip_t *zip = zip_open(path, 0, 'r');
			int i, n = zip_entries_total(zip);
			for (i = 0; i < n; ++i)
			{
				zip_entry_openbyindex(zip, i);
				if (strcmp(get_filename_ext(zip_entry_name(zip)), "dll") == 0)
				{
					PMEMORYMODULE handle = NULL;
					size_t sz = 0;
					void *buf = NULL;
					zip_entry_read(zip, &buf, &sz);
					handle = MemoryLoadLibrary(buf, sz);
					if (buf)
						free(buf);
					zip_entry_close(zip);
					zip_close(zip);
					return handle;
				}
			}
			zip_entry_close(zip);
			zip_close(zip);
			return NULL;
		}
		else
		{
			std::vector<uint8_t> dll_ptr = load_data(path);
			PMEMORYMODULE handle = MemoryLoadLibrary(dll_ptr.data(), dll_ptr.size());
			return handle;
		}
#else
		void *handle = SDL_LoadObject(path);
		return (!handle) ? NULL : handle;
#endif

#else
		void *handle = SDL_LoadObject(path);
		return (!handle) ? NULL : handle;
#endif
	}
	void *getfunc(void *handle, const char *funcname)
	{
#ifdef _WIN32
#ifndef DEBUG
		return (void *)MemoryGetProcAddress((PMEMORYMODULE)handle, funcname);
#else
		return SDL_LoadFunction(handle, funcname);
#endif
#else
		return SDL_LoadFunction(handle, funcname);
#endif
	}
	void freelib(void *handle)
	{
#ifdef _WIN32
#ifndef DEBUG
		MemoryFreeLibrary((PMEMORYMODULE)handle);
#else
		SDL_UnloadObject(handle);
#endif
#else
		SDL_UnloadObject(handle);
#endif
	}

	const char *b64tb = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string base64_encode(const std::string &in)
	{
		std::string out;
		int val = 0, valb = -6;
		for (unsigned char c : in)
		{
			val = (val << 8) + c;
			valb += 8;
			while (valb >= 0)
			{
				out.push_back(b64tb[(val >> valb) & 0x3F]);
				valb -= 6;
			}
		}
		if (valb > -6)
			out.push_back(b64tb[((val << 8) >> (valb + 8)) & 0x3F]);
		while (out.size() % 4)
			out.push_back('=');
		return out;
	}

	std::string base64_decode(const std::string &in)
	{
		std::string out;
		std::vector<int> T(256, -1);
		for (int i = 0; i < 64; i++)
			T[b64tb[i]] = i;
		int val = 0, valb = -8;
		for (unsigned char c : in)
		{
			if (T[c] == -1)
				break;
			val = (val << 6) + T[c];
			valb += 6;
			if (valb >= 0)
			{
				out.push_back(char((val >> valb) & 0xFF));
				valb -= 8;
			}
		}
		return out;
	}

	MEM *mopen(const int8_t *src, uint32_t length)
	{
		MEM *b;

		if ((src == NULL) || (length <= 0))
			return (NULL);

		b = (MEM *)(malloc(sizeof(MEM)));
		if (b == NULL)
			return (NULL);

		b->_base = (int8_t *)(src);
		b->_ptr = (int8_t *)(src);
		b->_cnt = length;
		b->_bufsiz = length;
		b->_eof = 0;

		return (b);
	}

	void mclose(MEM *buf)
	{
		if (buf != NULL)
		{
			free(buf);
			buf = NULL;
		}
	}

	int32_t mtell(MEM *buf)
	{
		return (buf->_bufsiz - buf->_cnt);
	}

	size_t mread(void *buffer, size_t size, size_t count, MEM *buf)
	{
		size_t wrcnt;
		int32_t pcnt;

		if (buf == NULL)
			return (0);
		if (buf->_ptr == NULL)
			return (0);

		wrcnt = size * count;
		if ((size == 0) || buf->_eof)
			return (0);

		pcnt = ((uint32_t)(buf->_cnt) > wrcnt) ? wrcnt : buf->_cnt;
		memcpy(buffer, buf->_ptr, pcnt);

		buf->_cnt -= pcnt;
		buf->_ptr += pcnt;

		if (buf->_cnt <= 0)
		{
			buf->_ptr = buf->_base + buf->_bufsiz;
			buf->_cnt = 0;
			buf->_eof = 1;
		}

		return (pcnt / size);
	}

	size_t mwrite(const void *buffer, size_t size, size_t count, MEM *buf)
	{
		size_t wrcnt;
		int32_t pcnt;

		if (buf == NULL)
			return (0);
		if (buf->_ptr == NULL)
			return (0);

		wrcnt = size * count;
		if ((size == 0) || buf->_eof)
			return (0);

		pcnt = ((uint32_t)(buf->_cnt) > wrcnt) ? wrcnt : buf->_cnt;
		memcpy(buf->_ptr, buffer, pcnt);

		buf->_cnt -= pcnt;
		buf->_ptr += pcnt;

		if (buf->_cnt <= 0)
		{
			buf->_ptr = buf->_base + buf->_bufsiz;
			buf->_cnt = 0;
			buf->_eof = 1;
		}

		return (pcnt / size);
	}

	int32_t meof(MEM *buf)
	{
		if (buf == NULL)
			return (1); // XXX: Should return a different value?

		return (buf->_eof);
	}

	void mseek(MEM *buf, int32_t offset, int32_t whence)
	{
		if (buf == NULL)
			return;

		if (buf->_base)
		{
			switch (whence)
			{
			case SEEK_SET:
				buf->_ptr = buf->_base + offset;
				break;
			case SEEK_CUR:
				buf->_ptr += offset;
				break;
			case SEEK_END:
				buf->_ptr = buf->_base + buf->_bufsiz + offset;
				break;
			default:
				break;
			}

			buf->_eof = 0;
			if (buf->_ptr >= (buf->_base + buf->_bufsiz))
			{
				buf->_ptr = buf->_base + buf->_bufsiz;
				buf->_eof = 1;
			}

			buf->_cnt = (buf->_base + buf->_bufsiz) - buf->_ptr;
		}
	}

	MEMMAP *memmap_open(const char *fname, size_t *sz)
	{
		MEMMAP *b;

		if ((fname == NULL))
			return (NULL);

		b = (MEMMAP *)(malloc(sizeof(MEMMAP)));
		if (b == NULL)
			return (NULL);
		memset(b, 0, sizeof(MEMMAP));
		b->_mappedsize = 0;

#ifdef _WIN32
		// open file
		b->_file = ::CreateFileA(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (!b->_file)
		{
			memmap_close(b);
			return (NULL);
		}

		// file size
		LARGE_INTEGER result;
		if (!GetFileSizeEx(b->_file, &result))
		{
			memmap_close(b);
			return (NULL);
		}

		b->_bufsiz = static_cast<uint64_t>(result.QuadPart);
		*sz = b->_bufsiz;

		// convert to mapped mode
		b->_mappedfile = ::CreateFileMapping(b->_file, NULL, PAGE_READONLY, 0, 0, NULL);
		if (!b->_mappedfile)
		{
			memmap_close(b);
			return (NULL);
		}

		b->_ptr = (uint8_t *)MapViewOfFile(b->_mappedfile, FILE_MAP_READ, 0, 0, b->_bufsiz);
		if (!b->_ptr)
		{
			memmap_close(b);
			return (NULL);
		}
		b->_base = b->_ptr;
		b->_cnt = b->_bufsiz;
		b->_bufsiz = b->_bufsiz;
		b->_eof = 0;
		return b;
#else
		b->_file = ::open(fname, O_RDONLY | O_LARGEFILE);
		if (b->_file == -1)
		{
			memmap_close(b);
			return (NULL);
		}

		// file size
		struct stat64 statInfo;
		if (fstat64(b->_file, &statInfo) < 0)
		{
			memmap_close(b);
			return (NULL);
		}

		b->_bufsiz = statInfo.st_size;
		b->_ptr = (uint8_t *)mmap64(NULL, b->_bufsiz, PROT_READ, MAP_SHARED, b->_file, 0);
		b->_bufsiz = statInfo.st_size;
		b->_base = b->_ptr;
		b->_cnt = b->_bufsiz;
		b->_bufsiz = b->_bufsiz;
		b->_eof = 0;
		return b;
#endif
	}

	void memmap_close(MEMMAP *buf)
	{
		if (buf != NULL)
		{
#ifdef _WIN32
			if (buf->_ptr)
				::UnmapViewOfFile(buf->_ptr);
			if (buf->_mappedfile)
				::CloseHandle(buf->_mappedfile);
			if (buf->_file)
				::CloseHandle(buf->_file);
#else
			if (buf->ptr)
				munmap(buf->_ptr, buf->_bufsiz);
			if (buf->_file)
				close(buf->_file);
#endif
			free(buf);
			buf = NULL;
		}
	}
	size_t memmap_tell(MEMMAP *buf)
	{
		return (buf->_bufsiz - buf->_cnt);
	}
	size_t memmap_read(void *buffer, size_t size, size_t count, MEMMAP *buf)
	{
		size_t wrcnt;
		size_t pcnt;

		if (buf == NULL)
			return (0);
		if (buf->_ptr == NULL)
			return (0);

		wrcnt = size * count;
		if ((size == 0) || buf->_eof)
			return (0);

		pcnt = (buf->_cnt > wrcnt) ? wrcnt : buf->_cnt;
		memcpy(buffer, buf->_ptr, pcnt);

		buf->_cnt -= pcnt;
		buf->_ptr += pcnt;

		if (buf->_cnt <= 0)
		{
			buf->_ptr = buf->_base + buf->_bufsiz;
			buf->_cnt = 0;
			buf->_eof = 1;
		}

		return (pcnt / size);
	}
	size_t memmap_eof(MEMMAP *buf)
	{
		if (buf == NULL)
			return (1); // XXX: Should return a different value?

		return (buf->_eof);
	}
	void memmap_seek(MEMMAP *buf, size_t offset, int32_t whence)
	{
		if (buf == NULL)
			return;

		if (buf->_base)
		{
			switch (whence)
			{
			case SEEK_SET:
				buf->_ptr = buf->_base + offset;
				break;
			case SEEK_CUR:
				buf->_ptr += offset;
				break;
			case SEEK_END:
				buf->_ptr = buf->_base + buf->_bufsiz + offset;
				break;
			default:
				break;
			}

			buf->_eof = 0;
			if (buf->_ptr >= (buf->_base + buf->_bufsiz))
			{
				buf->_ptr = buf->_base + buf->_bufsiz;
				buf->_eof = 1;
			}

			buf->_cnt = (buf->_base + buf->_bufsiz) - buf->_ptr;
		}
	}

}