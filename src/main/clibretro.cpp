#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "io.h"
#include "utils.h"
#include <algorithm>
#define INI_IMPLEMENTATION
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

bool CLibretro::core_savestate(const char *filename, bool save)
{
  if (lr_isrunning)
  {
    size_t size = retro.retro_serialize_size();
    if (size)
    {
      auto Memory = std::make_unique<uint8_t[]>(size);
      if (save)
      {
        // Get the filesize
        retro.retro_serialize(Memory.get(), size);
        save_data(Memory.get(), size, filename);
      }
      else
      {
        unsigned sz;
        std::vector<uint8_t> save_data = load_data(filename, &sz);
        if (save_data.empty())
          return false;
        memcpy(Memory.get(), (uint8_t *)save_data.data(), size);
        retro.retro_unserialize(Memory.get(), size);
      }
      return true;
    }
  }
  return false;
}

bool CLibretro::core_saveram(const char *filename, bool save)
{
  if (lr_isrunning)
  {
    size_t size = retro.retro_get_memory_size(RETRO_MEMORY_SAVE_RAM);
    if (size)
    {
      uint8_t *Memory = (uint8_t *)retro.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
      if (Memory)
      {
        if (save)
          return save_data(Memory, size, filename);
        else
        {
          unsigned sz;
          std::vector<uint8_t> save_data = load_data(filename, &sz);
          if (save_data.empty())
            return false;
          memcpy(Memory, (uint8_t *)&save_data[0], size);
          return true;
        }
      }
    }
  }
  return false;
}

bool CLibretro::load_coresettings()
{
  int size_ = get_filesize(core_config.c_str());
  ini_t *ini = NULL;
  if (!size_)
  {

  // create a new file with defaults
  // create cache of core options
  redo:
    if (core_variables.size())
    {
      ini = ini_create(NULL);
      int section =
          ini_section_add(ini, "Core Settings", strlen("Core Settings"));
      for (size_t i = 0; i < core_variables.size(); i++)
      {
        ini_property_add(ini, section, (char *)core_variables[i].name.c_str(),
                         core_variables[i].name.length(),
                         (char *)core_variables[i].var.c_str(),
                         core_variables[i].var.length());

        for (size_t j = 0; j < core_variables[i].config_vals.size(); j++)
        {
          if (core_variables[i].config_vals[j] == core_variables[i].var)
          {
            core_variables[i].sel_idx = j;
            break;
          }
        }
      }
      std::string numvars = std::to_string(core_variables.size());
      ini_property_add(ini, section, "usedvars_num", strlen("usedvars_num"), numvars.c_str(), numvars.length());
      int size = ini_save(ini, NULL, 0); // Find the size needed
      auto ini_data = std::make_unique<char[]>(size);
      size = ini_save(ini, ini_data.get(), size); // Actually save the file
      save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
      ini_destroy(ini);
    }
    return false;
  }
  else
  {
    std::vector<uint8_t> data = load_data(core_config.c_str(), (unsigned int *)&size_);
    ini_t *ini = ini_load((char *)data.data(), NULL);
    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    if (section != INI_NOT_FOUND)
    {
      int idx = ini_find_property(ini, section, "usedvars_num", strlen("usedvars_num"));
      const char *numvars = ini_property_value(ini, section, idx);
      size_t vars_infile = atoi(numvars);
      if (core_variables.size() != vars_infile)
      {
        // rebuild cache.
        ini_destroy(ini);
        goto redo;
      }

      for (size_t i = 0; i < vars_infile; i++)
      {
        std::string name = ini_property_name(ini, section, i);
        std::string value = ini_property_value(ini, section, i);
        core_variables[i].name = name;
        core_variables[i].var = value;

        for (size_t j = 0; j < core_variables[i].config_vals.size(); j++)
        {
          if (core_variables[i].config_vals[j] == core_variables[i].var)
          {
            core_variables[i].sel_idx = j;
            break;
          }
        }
      }
    }
    ini_destroy(ini);
    return (section != INI_NOT_FOUND ? true : false);
  }
  return false;
}

void CLibretro::save_coresettings()
{
  unsigned sz_coreconfig = get_filesize(core_config.c_str());
  if (sz_coreconfig)
  {
    unsigned size_;
    std::vector<uint8_t> data = load_data((const char *)core_config.c_str(), &size_);
    ini_t *ini = ini_load((char *)data.data(), NULL);
    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    for (size_t i = 0; i < core_variables.size(); i++)
    {
      int idx = ini_find_property(ini, section,
                                  core_variables[i].name.c_str(),
                                  core_variables[i].name.length());
      ini_property_value_set(ini, section, idx, core_variables[i].var.c_str(),
                             core_variables[i].var.length());
    }
    std::string numvars = std::to_string(core_variables.size());
    int idx = ini_find_property(ini, section,
                                "usedvars_num", strlen("usedvars_num"));
    ini_property_value_set(ini, section, idx, numvars.c_str(),
                           numvars.length());
    int size = ini_save(ini, NULL, 0); // Find the size needed
    auto ini_data = std::make_unique<char[]>(size);
    size = ini_save(ini, ini_data.get(), size); // Actually save the file
    save_data((unsigned char *)ini_data.get(), size, core_config.c_str());
    ini_destroy(ini);
  }
}

bool CLibretro::init_inputvars(retro_input_descriptor *var)
{
  ::load_inpcfg(var);
  return true;
}

const char *CLibretro::load_corevars(retro_variable *var)
{
  for (size_t i = 0; i < core_variables.size(); i++)
  {

    if (strcmp(core_variables[i].name.c_str(), var->key) == 0)
      return core_variables[i].var.c_str();
  }
  return NULL;
}

bool CLibretro::init_configvars(retro_variable *var)
{
  size_t num_vars = 0;

  std::vector<loadedcore_configvars> variables;
  variables.clear();
  variables_changed = false;

  for (const struct retro_variable *v = var; v->key; ++v)
    num_vars++;

  for (unsigned i = 0; i < num_vars; ++i)
  {
    loadedcore_configvars vars_struct;
    const struct retro_variable *invar = &var[i];

    vars_struct.name = invar->key;
    vars_struct.config_visible = true;
    std::string str1 = invar->value;
    std::string::size_type pos = str1.find(';');
    vars_struct.description = str1.substr(0, pos);
    // skip ; and space to first variable
    pos = str1.find(' ', pos) + 1;
    // get all variables
    str1 = str1.substr(pos, string::npos);
    vars_struct.usevars = str1;

    std::stringstream test(str1);
    std::string segment;
    std::vector<std::string> seglist;
    while (std::getline(test, segment, '|'))
      vars_struct.config_vals.push_back(segment);

    pos = str1.find('|');
    // get first variable as default/current
    if (pos != std::string::npos)
      str1 = str1.substr(0, pos);
    vars_struct.var = str1;

    vars_struct.sel_idx = 0;
    variables.push_back(vars_struct);
  }

  core_variables = variables;

  load_coresettings();

  return true;
}

CLibretro::CLibretro(SDL_Window *window,char *exepath)
{
  cores.clear();
  get_cores(exepath);
  sdl_window = window;
  lr_isrunning = false;
  info = {0};
}

CLibretro::~CLibretro()
{
  core_unload();
}

bool CLibretro::core_load(char *ROM, bool game_specific_settings, char *corepath)
{
  std::filesystem::path romzpath = ROM;
  std::filesystem::path core_path_ = corepath;
  std::filesystem::path save_path_ = std::filesystem::path(exe_path) / "system";
  std::filesystem::path save_path = save_path_ / (romzpath.stem().string() + ".sram");
  romsavesstatespath = std::filesystem::absolute(save_path).string();
  saves_path = std::filesystem::absolute(save_path_).string();


  if (game_specific_settings)
    save_path.replace_filename(romzpath.stem().string() + ".corecfg");
  else
    save_path.replace_filename(core_path_.stem().string() + ".corecfg");
  core_config = std::filesystem::absolute(save_path).string();

  if (lr_isrunning)
    core_unload();

  void *hDLL = SDL_LoadObject((const char *)corepath);
  if (!hDLL)
    return false;

#define libload(name) SDL_LoadFunction(hDLL, name)
#define load_sym(V, name)                         \
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
  load_envsymb(retro.handle,true);
  retro.retro_init();
  load_envsymb(retro.handle,false);
  
  

  struct retro_system_info system = {0};
  retro_system_av_info av = {0};
  info = {ROM, 0};
  info.path = ROM;
  info.data = NULL;
  info.size = get_filesize(ROM);
  info.meta = "";





  if (!system.need_fullpath)
  {
    std::ifstream ifs;
    ifs.open(ROM, ios::binary);
    if (!ifs.good())
    {
    fail:
      printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
      return false;
    }
    info.data = malloc(info.size);
    if (!info.data)
      goto fail;
    ifs.read((char *)info.data, info.size);
  }

  if (!retro.retro_load_game(&info))
  {
    printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
    return false;
  }
  retro.retro_get_system_info(&system);
  retro.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
  retro.retro_get_system_av_info(&av);
  
  SDL_DisplayMode dm;
  SDL_GetDesktopDisplayMode(0, &dm);
  int swap =1;
  if (dm.refresh_rate % (int)av.timing.fps == 0) {
    swap = dm.refresh_rate / (int)av.timing.fps;
  }
  SDL_GL_SetSwapInterval(swap);
  audio_init(dm.refresh_rate, av.timing.sample_rate, av.timing.fps);
  video_init(&av.geometry, sdl_window);
  lr_isrunning = true;

  core_saveram(romsavesstatespath.c_str(), false);
  return true;
}

bool CLibretro::core_isrunning()
{
  return lr_isrunning;
}

void CLibretro::core_run()
{
  retro.retro_run();
}

void CLibretro::core_unload()
{
  if (lr_isrunning)
  {
    core_saveram(romsavesstatespath.c_str(), true);
    if(retro.handle != NULL)
    {
    audio_destroy();
    video_deinit();
    retro.retro_unload_game();
    retro.retro_deinit();
    SDL_UnloadObject(retro.handle);
    retro.handle = NULL;
    memset((retro_core *)&retro, 0, sizeof(retro_core));
    }
  
  lr_isrunning = false;  
  }
}

void CLibretro::get_cores(char* exepath)
{
  std::filesystem::path p(exepath);
  exe_path = p.parent_path().native();
  std::filesystem::path path = p.parent_path() / "cores";
  for (auto &entry : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
  {
    string str = entry.path().string();
    if (entry.is_regular_file() && entry.path().extension() == SHLIB_EXTENSION)
    {
      typedef void (*retro_get_system_info)(struct retro_system_info * info);
      retro_get_system_info getinfo;
      struct retro_system_info system = {0};
      void *hDLL = SDL_LoadObject(str.c_str());
      if (!hDLL)
      {
        continue;
      }
      getinfo = (retro_get_system_info)SDL_LoadFunction(hDLL, "retro_get_system_info");
      if (getinfo)
      {
        getinfo(&system);
        core_info entry;
        entry.fps = 60;
        entry.samplerate = 44100;
        entry.aspect_ratio = 4 / 3;
        entry.core_name = system.library_name;
        std::string ext = system.valid_extensions;
        entry.core_extensions = ext;
        entry.core_path = str;
        cores.push_back(entry);
        SDL_UnloadObject(hDLL);
      }
      
    }
  }
  coreexts = "All supported {.";
  for (auto &corez : cores)
  {
    std::stringstream test(corez.core_extensions);
    std::string segment;
    std::vector<std::string> seglist;
    while (std::getline(test, segment, '|'))
      if (coreexts.find(segment) == std::string::npos)
        coreexts += segment + ",.";
  }
  coreexts.resize(coreexts.size() - 2);
  coreexts += "}";
}