#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "io.h"
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
  static CLibretro* instance = new CLibretro(window);
  return instance;
}

CLibretro::CLibretro(void *window)
{
  memset((retro_core*)&retro,0,sizeof(retro_core));
  cores.clear();
  get_cores();
  render = create_gl_render(window);
}

CLibretro::~CLibretro()
{
  freelib(retro.handle);
  memset((retro_core*)&retro,0,sizeof(retro_core));
  cores.clear();
  delete render;
}

bool CLibretro::core_load(char* ROM, bool game_specific_settings)
{
  const char* str = cores.at(0).core_path.c_str();
  void* hDLL =openlib((const char*)str);
  if (!hDLL) { return false; }
  #define libload(name) getfunc(hDLL, name)
  #define load_sym(V, name) \
  if (!(*(void **)(&V) = (void *)libload(#name))) \
  return false;
#define load_retro_sym(S) load_sym(retro.S, S)
  retro.handle = hDLL;
  load_retro_sym(retro_init);
  load_retro_sym(retro_deinit);
  load_retro_sym(retro_api_version);
  load_retro_sym(retro_get_system_info);
  load_retro_sym(retro_get_system_av_info);
  load_retro_sym(retro_set_controller_port_device);
  load_retro_sym(retro_reset);
  load_retro_sym(retro_run);
  load_retro_sym(retro_load_game);
  load_retro_sym(retro_unload_game);
  load_retro_sym(retro_serialize);
  load_retro_sym(retro_unserialize);
  load_retro_sym(retro_serialize_size);
  load_retro_sym(retro_get_memory_size);
  load_retro_sym(retro_get_memory_data);
  
}

bool CLibretro::core_isrunning()
{
  return lr_isrunning;
}

void CLibretro::core_run()
{
  if(lr_isrunning)
  {
  int i=0;
  i+= 3;
  i-= 3;
  }
}



 void addplugin(const char *path,std::vector<core_info> *cores) {
    typedef void (*retro_get_system_info)(struct retro_system_info * info);
    retro_get_system_info getinfo;
    struct retro_system_info system = {0};

  void* hDLL =openlib(path);
  if (!hDLL) { return; }
      getinfo = (retro_get_system_info)getfunc(hDLL,"retro_get_system_info");
      if (getinfo) {
        getinfo(&system);
        core_info entry;
        entry.fps = 60;
        entry.samplerate = 44100;
        entry.aspect_ratio = 4/3;

        entry.core_name = system.library_name;
        entry.core_extensions = system.valid_extensions;
        entry.core_path = path;
        cores->push_back(entry);
      }
      freelib(hDLL);
  }

void CLibretro::get_cores()
{
    std::filesystem::path path =std::filesystem::current_path() / "cores";
    for (auto & entry : std::filesystem::directory_iterator(path))
    {
      string str = entry.path().string();
      if(entry.is_regular_file() && entry.path().extension() == SHLIB_EXTENSION)
          addplugin(entry.path().string().c_str(),&cores);
    }
}