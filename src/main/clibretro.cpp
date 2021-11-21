#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

void* openlib(char *path)
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
void* getfunc(void *handle,char* funcname)
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

static CLibretro* get_classinstance(void* window)
{
  static thread_local CLibretro* instance = new CLibretro(window);
  return instance;
}

CLibretro::CLibretro(void *window)
{
  string path = std::filesystem::current_path().string();
  path += "//cores";
  cores = get_cores(path.c_str());
}



 void addplugin(char *path,std::vector<core_info> cores) {
    typedef void (*retro_get_system_info)(struct retro_system_info * info);
    retro_get_system_info getinfo;
    struct retro_system_info system = {0};

  void* hDLL =openlib(path);
  if (!hDLL) { return; }
      getinfo = (retro_get_system_info)getfunc(hDLL,"retro_get_system_info");
      if (getinfo) {
        getinfo(&system);
        core_info entry = {0};
        entry.core_name = system.library_name;
        entry.core_extensions = system.valid_extensions;
        entry.core_path = path;
        cores.push_back(entry);
      }
      freelib(hDLL);
  }

std::vector<core_info> get_cores(const char* path)
{
    std::vector<core_info> core_dat;
    core_dat.clear();

    for (const auto & entry : std::filesystem::directory_iterator(path))
    {
      std::cout << entry.path() << std::endl;

    }
    return core_dat;
}