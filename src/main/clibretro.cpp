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


static void core_audio_sample(int16_t left, int16_t right) {
  CLibretro *lib = CLibretro::get_classinstance();
  if (lib->core_isrunning()) {
    int16_t buf[2] = {left, right};
    audio_mix(buf, 1);
  }
}
static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
  CLibretro *lib = CLibretro::get_classinstance();
  if (lib->core_isrunning()) {
    audio_mix(data, frames);
  }
  return frames;
}

static void core_log(enum retro_log_level level, const char *fmt, ...) {
  char buffer[4096] = {0};
  static const char *levelstr[] = {"dbg", "inf", "wrn", "err"};
  va_list va;
  va_start(va, fmt);
  vsnprintf(buffer, sizeof(buffer), fmt, va);
  va_end(va);
  if (level == 0)
    return;
  fprintf(stdout, "[%s] %s", levelstr[level], buffer);
}

static bool core_environment(unsigned cmd, void *data) {
  bool *bval;
  CLibretro *retro = CLibretro::get_classinstance();
  switch (cmd) {
  case RETRO_ENVIRONMENT_SET_MESSAGE: {
    struct retro_message *cb = (struct retro_message *)data;
    return true;
  }
  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
    struct retro_log_callback *cb = (struct retro_log_callback *)data;
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY: {
      struct retro_core_option_display* cb = (struct retro_core_option_display*)data;
      return false;
      break;
  }

  case RETRO_ENVIRONMENT_GET_FASTFORWARDING: {
    *(bool *)data = false;
    return true;
  } break;

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval = (bool *)data;
    *bval = true;
    return true;
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
  {
    static char *sys_path = NULL;
    if (!sys_path) {
      std::filesystem::path path =std::filesystem::current_path() / "system";
      sys_path = strdup(path.string().c_str());
    }
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }

  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 31
  {
      struct retro_input_descriptor* var =
          (struct retro_input_descriptor*)data;
    return true;
  }

  break;

  case RETRO_ENVIRONMENT_SET_VARIABLES: {
    struct retro_variable *var = (struct retro_variable *)data;
    return false;
  } break;

  case RETRO_ENVIRONMENT_GET_VARIABLE: {
    struct retro_variable *variable = (struct retro_variable *)data;
    return false;
  } break;

  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
    return false;
  } break;

  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO: {
      return false;
      break;
  }

  case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION: {
      return false;
  }

  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
    enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
    return video_set_pixelformat(*fmt);
  }
  case RETRO_ENVIRONMENT_SET_HW_RENDER: {
      return false;
  }
  default:
    core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
    return false;
  }
  return false;
}

static void core_video_refresh(const void *data, unsigned width,
                               unsigned height, size_t pitch) {
    CLibretro* retro = CLibretro::get_classinstance();
    if (retro->core_isrunning())
    video_refresh(data, width, height, pitch);
}

static void core_input_poll(void) {
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index,
                                unsigned id) {
  return 0;
}




CLibretro *CLibretro::get_classinstance(SDL_Window* window)
{
  static CLibretro* instance = new CLibretro(window);
  return instance;
}

CLibretro::CLibretro(SDL_Window *window)
{
  memset((retro_core*)&retro,0,sizeof(retro_core));
  cores.clear();
  get_cores();
  sdl_window = window;
}

CLibretro::~CLibretro()
{
  core_unload();
 
}

static unsigned get_filesize(FILE* fp)
	{
		unsigned size = 0;
		unsigned pos = ftell(fp);
		fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		fseek(fp, pos, SEEK_SET);
		return size;
	}

static unsigned get_filesize(const char *path)
	{
		auto input = unique_ptr<FILE, int(*)(FILE*)>(fopen(path, "rb"), &fclose);
		if (!input)return NULL;
		unsigned size = get_filesize(input.get());
		return size;
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
  load_retro_sym(retro_set_environment);
  load_retro_sym(retro_set_video_refresh);
  load_retro_sym(retro_set_input_poll);
  load_retro_sym(retro_set_input_state);
  load_retro_sym(retro_set_audio_sample);
  load_retro_sym(retro_set_audio_sample_batch);
  retro.retro_set_environment(core_environment);
  retro.retro_set_video_refresh(core_video_refresh);
  retro.retro_set_input_poll(core_input_poll);
  retro.retro_set_input_state(core_input_state);
  retro.retro_set_audio_sample(core_audio_sample);
  retro.retro_set_audio_sample_batch(core_audio_sample_batch);
  retro.retro_init();
  retro.initialized = true;

  struct retro_system_info system = {0};
  retro_system_av_info av = {0};
  info = {ROM, 0};
  info.path = ROM;
  info.data = NULL;
  info.size = get_filesize(ROM);
  info.meta = "";

  retro.retro_get_system_info(&system);
  if (!system.need_fullpath) {
    FILE *inputfile = fopen(ROM, "rb");
    if (!inputfile) {
    fail:
      printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
      return false;
    }
    info.data = malloc(info.size);
    if (!info.data)
      goto fail;
    size_t read = fread((void *)info.data, 1, info.size, inputfile);
    fclose(inputfile);
    inputfile = NULL;
  }

   if (!retro.retro_load_game(&info)) {
    printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
    return false;
  }
  retro.retro_get_system_av_info(&av);
  retro.retro_set_controller_port_device(0, 0);
  float refreshr = 60;
  audio_init(refreshr, av.timing.sample_rate, av.timing.fps);
  video_init(&av.geometry,refreshr,sdl_window);
  lr_isrunning= true;
  return true;
  
}

bool CLibretro::core_isrunning()
{
  return lr_isrunning;
}

void CLibretro::core_run()
{
  if(lr_isrunning)
  {
  video_buf_clear();
  retro.retro_run();
  }
}

void CLibretro::core_unload() {
  if (retro.initialized) {
    retro.retro_unload_game();
    retro.retro_deinit();
  }
  if (retro.handle) {
    freelib(retro.handle);
    retro.handle = NULL;
    memset((retro_core*)&retro,0,sizeof(retro_core));
  }
  if (info.data)
      free((void *)info.data);
    audio_destroy();

  cores.clear();
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