#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

#ifdef _WIN32
static std::string_view SHLIB_EXTENSION = ".dll";
#else
static std::string_view SHLIB_EXTENSION = ".so";
#endif

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



CLibretro *CLibretro::get_classinstance(void* window)
{
  static thread_local CLibretro* instance = new CLibretro(window);
  return instance;
}

CLibretro::CLibretro(void *window)
{
  
  cores = get_cores();
}

void CLibretro::core_run()
{
  int i=0;
  i+= 3;
  i-= 3;
  return;

}



 void addplugin(const char *path,std::vector<core_info> cores) {
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

std::vector<core_info> get_cores()
{
    std::vector<core_info> core_dat;
    core_dat.clear();
    std::filesystem::path path =std::filesystem::current_path() / "cores";
    for (auto & entry : std::filesystem::directory_iterator(path))
    {
      string str = entry.path().string();
      if(entry.is_regular_file() && entry.path().extension() == SHLIB_EXTENSION)
          addplugin(entry.path().string().c_str(),core_dat);
    }
    return core_dat;
}