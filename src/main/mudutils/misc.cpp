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
#endif
#include <unistd.h>
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
		void *handle = SDL_LoadObject(path);
		if (!handle)
			return NULL;
		return handle;
	}
	void *getfunc(void *handle, const char *funcname)
	{
		return SDL_LoadFunction(handle, funcname);
	}
	void freelib(void *handle)
	{
		SDL_UnloadObject(handle);
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

}