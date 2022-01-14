#include "utils.h"
#include <iostream>
#include <fstream>
#include <string>
#include <iostream>
#include <filesystem>
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#ifdef _WIN32

#endif
using namespace std;

#ifdef _WIN32
#include <windows.h>
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