#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "io.h"
#include "utils.h"
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

CLibretro *CLibretro::get_classinstance(SDL_Window *window)
{
  static thread_local CLibretro *instance2 = new CLibretro(window);
  return instance2;
}

void CLibretro::poll()
{
  poll_lr();
}

bool CLibretro::load_coresettings()
{
  size_t lastindex = core_path.find_last_of(".");
  std::string core_config = core_path.substr(0, lastindex) + ".corecfg";
  int size_ = get_filesize(core_config.c_str());
  ini_t *ini = NULL;
  if (!size_)
  {

  // create a new file with defaults
  // create cache of core options
  redo:
    ini = ini_create(NULL);
    int section =
        ini_section_add(ini, "Core Settings", strlen("Core Settings"));
    for (int i = 0; i < core_variables.size(); i++)
    {
      ini_property_add(ini, section, (char *)core_variables[i].name.c_str(),
                       core_variables[i].name.length(),
                       (char *)core_variables[i].var.c_str(),
                       core_variables[i].var.length());

      for (int j = 0; j < core_variables[i].config_vals.size(); j++)
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
    return false;
  }
  else
  {
    std::vector<uint8_t> data = load_data(core_config.c_str(), (unsigned int *)&size_);
    ini_t *ini = ini_load((char *)data.data(), NULL);
    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    int idx = ini_find_property(ini, section, "usedvars_num", strlen("usedvars_num"));
    const char *numvars = ini_property_value(ini, section, idx);
    int vars_infile = atoi(numvars);
    if (core_variables.size() != vars_infile)
    {
      // rebuild cache.
      ini_destroy(ini);
      goto redo;
    }

    for (int i = 0; i < vars_infile; i++)
    {
      std::string name = ini_property_name(ini, section, i);
      std::string value = ini_property_value(ini, section, i);
      core_variables[i].name = name;
      core_variables[i].var = value;

      for (int j = 0; j < core_variables[i].config_vals.size(); j++)
      {
        if (core_variables[i].config_vals[j] == core_variables[i].var)
        {
          core_variables[i].sel_idx = j;
          break;
        }
      }
    }
    ini_destroy(ini);
    return true;
  }
  return false;
}

void CLibretro::save_coresettings()
{
  size_t lastindex = core_path.find_last_of(".");
  std::string core_config = core_path.substr(0, lastindex) + ".corecfg";

  unsigned sz_coreconfig = get_filesize(core_config.c_str());
  if (sz_coreconfig)
  {
    unsigned size_;
    std::vector<uint8_t> data = load_data((const char *)core_config.c_str(), &size_);
    ini_t *ini = ini_load((char *)data.data(), NULL);
    int section = ini_find_section(ini, "Core Settings", strlen("Core Settings"));
    for (int i = 0; i < core_variables.size(); i++)
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
  core_inputbinds.clear();

  int numvars = 0, i = 0;

  while (var->description != NULL && var->port == 0)
  {

    if (var->device == RETRO_DEVICE_ANALOG || (var->device == RETRO_DEVICE_JOYPAD))
    {

      coreinput_bind bind;

      bind.description = var->description;

      if (var->device == RETRO_DEVICE_ANALOG)
      {
        int var_index = var->index;
        int axistocheck = var->id;
        if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (var->id == RETRO_DEVICE_ID_ANALOG_X))
          axistocheck = joypad_analogx_l;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_LEFT) && (var->id == RETRO_DEVICE_ID_ANALOG_Y))
          axistocheck = joypad_analogy_l;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (var->id == RETRO_DEVICE_ID_ANALOG_X))
          axistocheck = joypad_analogx_r;
        else if ((var_index == RETRO_DEVICE_INDEX_ANALOG_RIGHT) && (var->id == RETRO_DEVICE_ID_ANALOG_Y))
          axistocheck = joypad_analogy_r;

        bind.retro_id = axistocheck;
        bind.isanalog = true;
        bind.sdl_id = 0;
        bind.joytype = joytype_::keyboard;
        bind.joykey_desc = "None";
      }
      else if (var->device == RETRO_DEVICE_JOYPAD)
      {
        bind.retro_id = var->id;
        bind.sdl_id = 0;
        bind.joytype = joytype_::keyboard;
        bind.isanalog = false;
        bind.joykey_desc = "None";
      }
      core_inputbinds.push_back(bind);
      var++;
    }
  }
  ::load_inpcfg();
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

CLibretro::CLibretro(SDL_Window *window)
{
  memset((retro_core *)&retro, 0, sizeof(retro_core));
  cores.clear();
  get_cores();
  sdl_window = window;
  lr_isrunning = false;
  info = {0};
}

CLibretro::~CLibretro()
{
  core_unload();
}

bool CLibretro::core_load(char *ROM, bool game_specific_settings)
{
  if (lr_isrunning)
  {
    lr_isrunning = false;
    core_unload();
  }

  std::string c;
  for (int i = 0; cores.size(); i++)
  {
    c = cores.at(i).core_path;
    std::string ext = ROM;
    ext = ext.substr(ext.find_last_of(".") + 1);
    if (ext == "sfc")
      if (c.find("snes9x_libretro") != std::string::npos)
        break;

    if (ext == "n64" || ext == "z64" || ext == "v64")
      if (c.find("mupen64plus_next_libretro") != std::string::npos)
        break;

    if (ext == "chd")
      if (c.find("mednafen_psx_hw_libretro") != std::string::npos)
        break;
  }

  core_path = c;
  void *hDLL = openlib((const char *)c.c_str());
  if (!hDLL)
  {
    return false;
  }

#define libload(name) getfunc(hDLL, name)
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
  load_envsymb(retro.handle);
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
  retro.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
  if (!system.need_fullpath)
  {
    FILE *inputfile = fopen(ROM, "rb");
    if (!inputfile)
    {
    fail:
      printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
      return false;
    }
    info.data = malloc(info.size);
    if (!info.data)
      goto fail;
    fread((void *)info.data, 1, info.size, inputfile);
    fclose(inputfile);
    inputfile = NULL;
  }

  if (!retro.retro_load_game(&info))
  {
    printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
    return false;
  }
  retro.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
  retro.retro_get_system_av_info(&av);
  float refreshr = 60;
  audio_init(refreshr, av.timing.sample_rate, av.timing.fps);
  video_init(&av.geometry, refreshr, sdl_window);
  lr_isrunning = true;
  return true;
}

bool CLibretro::core_isrunning()
{
  return lr_isrunning;
}

void CLibretro::core_run()
{
  if (lr_isrunning)
  {

    retro.retro_run();
  }
}

void CLibretro::core_unload()
{
  if (retro.initialized)
  {
    retro.retro_unload_game();
    retro.retro_deinit();
  }
  if (retro.handle)
  {
    freelib(retro.handle);
    retro.handle = NULL;
    memset((retro_core *)&retro, 0, sizeof(retro_core));
  }
  if (info.data)
    free((void *)info.data);
  audio_destroy();
  video_deinit();
}

void addplugin(const char *path, std::vector<core_info> *cores)
{
  typedef void (*retro_get_system_info)(struct retro_system_info * info);
  retro_get_system_info getinfo;
  struct retro_system_info system = {0};

  void *hDLL = openlib(path);
  if (!hDLL)
  {
    return;
  }
  getinfo = (retro_get_system_info)getfunc(hDLL, "retro_get_system_info");
  if (getinfo)
  {
    getinfo(&system);
    core_info entry;
    entry.fps = 60;
    entry.samplerate = 44100;
    entry.aspect_ratio = 4 / 3;

    entry.core_name = system.library_name;
    entry.core_extensions = system.valid_extensions;
    entry.core_path = path;
    cores->push_back(entry);
  }
  freelib(hDLL);
}

void CLibretro::get_cores()
{
  std::filesystem::path path = std::filesystem::current_path() / "cores";
  romsavesstatespath = path.generic_string();

  for (auto &entry : std::filesystem::directory_iterator(path))
  {
    string str = entry.path().string();
    if (entry.is_regular_file() && entry.path().extension() == SHLIB_EXTENSION)
      addplugin(entry.path().string().c_str(), &cores);
  }
}