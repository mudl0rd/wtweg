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

bool CLibretro::core_savestateslot(bool save)
{
  if (rom_path != "")
  {
    std::filesystem::path romzpath = std::filesystem::path(rom_path);
    std::filesystem::path save_path_ = std::filesystem::path(exe_path) / "saves";
    std::filesystem::path save_path = save_path_ / (romzpath.stem().string() + "_" + std::to_string(save_slot) + ".sram");
    std::string saves = std::filesystem::absolute(save_path).string();
    core_savestate(saves.c_str(), save);
    return true;
  }
  else
    return false;
}

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
        std::vector<uint8_t> save_ptr = load_data(filename, &sz);
        if ( save_ptr.empty())
          return false;
        memcpy(Memory.get(), save_ptr.data(), sz);
        retro.retro_unserialize(Memory.get(), sz);
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
    uint8_t *Memory = (uint8_t *)retro.retro_get_memory_data(RETRO_MEMORY_SAVE_RAM);
    if (!size || !Memory)
      return false;
    if (save)
      return save_data(Memory, size, filename);
    else
    {
      unsigned sz;
      std::vector<uint8_t> save_ptr = load_data(filename, &sz);
      if (save_ptr.empty())
        return false;
      if (sz != size)
        return false;
      memcpy(Memory, save_ptr.data(), sz);
      return true;
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
    ini = ini_load((char *)data.data(), NULL);
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

bool CLibretro::init_configvars_coreoptions(void *var, int version)
{
  core_variables.clear();
  core_categories.clear();
  variables_changed = false;

  if (version > 1)
  {
    auto *var3 = (struct retro_core_options_v2 *)var;
    retro_core_option_v2_category *var2 = var3->categories;
    retro_core_option_v2_definition *var1 = var3->definitions;

    while (var2->key != NULL)
    {
      loadedcore_configcat cat;
      cat.desc = var2->desc;
      cat.info = var2->info;
      cat.key = var2->key;
      cat.visible = true;
      core_categories.push_back(cat);
      var2++;
    }

    while (var1->key != NULL)
    {
      retro_core_option_value *values = var1->values;
      loadedcore_configvars vars_struct;
      vars_struct.sel_idx = 0;
      if (var1->category_key)
        vars_struct.category_name = var1->category_key;

      while (values->value != NULL)
      {
        vars_struct.config_vals.push_back(values->value);
        if (values->label)
        vars_struct.config_vals_desc.push_back(values->label);
        values++;
      }

      if (var1->default_value)
      {
        vars_struct.default_val = var1->default_value;
        vars_struct.var = var1->default_value;
      }
      else
      {

        vars_struct.var = vars_struct.config_vals[vars_struct.sel_idx];
      }

      

      if (var1->desc)
        vars_struct.description = var1->desc;
      if (var1->info)
        vars_struct.tooltip = var1->info;
      if (var1->info_categorized)
        vars_struct.tooltip_cat = var1->info_categorized;
      if (var1->desc_categorized)
        vars_struct.desc_cat = var1->desc_categorized;
      vars_struct.name = var1->key;
      vars_struct.config_visible = true;

      core_variables.push_back(vars_struct);
      var1++;
    }
    v2_vars = true;
  }
  else
  {
    auto *var3 = (struct retro_core_option_definition *)var;
    while (var3->key != NULL)
    {
      retro_core_option_value *var1 = var3->values;
      loadedcore_configvars vars_struct;
      vars_struct.sel_idx = 0;
      vars_struct.var = var3->default_value;
      vars_struct.description = var3->desc;
      if (var3->info)
        vars_struct.tooltip = var3->info;
      vars_struct.name = var3->key;
      vars_struct.config_visible = true;

      while (var1->value != NULL)
      {
        vars_struct.config_vals.push_back(var1->value);
        if (var1->label)
          vars_struct.config_vals_desc.push_back(var1->label);
        var1++;
      }
      core_variables.push_back(vars_struct);
      var3++;
    }
    v2_vars = false;
  }
  load_coresettings();
  return false;
}

bool CLibretro::init_configvars(retro_variable *var)
{
  core_variables.clear();
  variables_changed = false;

  while (var->key != NULL)
  {
    loadedcore_configvars vars_struct;
    const struct retro_variable *invar = var;

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
    while (std::getline(test, segment, '|'))
      vars_struct.config_vals.push_back(segment);

    pos = str1.find('|');
    // get first variable as default/current
    if (pos != std::string::npos)
      str1.resize(pos);
    vars_struct.var = str1;
    vars_struct.sel_idx = 0;
    core_variables.push_back(vars_struct);
    var++;
  }

  load_coresettings();
  v2_vars = false;
  return true;
}

void CLibretro::reset()
{
  lr_isrunning = false;
  save_slot = 0;
  variables_changed = false;
  perf_counter_last = 0;
  refreshrate = 0;
  controller_type[0] = RETRO_DEVICE_JOYPAD;
  controller_type[1] = RETRO_DEVICE_JOYPAD;
  // Assume "RetroPad"....fuck me
  disk_intf.clear();
  core_inputbinds[0].clear();
  core_inputbinds[1].clear();
  core_variables.clear();
  core_inputdesc[0].clear();
  core_inputdesc[1].clear();
  v2_vars = false;
  memset(&retro,0,sizeof(retro));
}

CLibretro::CLibretro(SDL_Window *window, char *exepath)
{
  reset();
  std::filesystem::path exe(get_wtfwegname());
  cores.clear();

  const char *dirs[3] = {"cores", "system", "saves"};
  for (int i = 0; i < 3; i++)
  {
    std::filesystem::path p = exe.parent_path().string();
    std::filesystem::path path = p / dirs[i];
    std::filesystem::create_directory(path);
  }
  get_cores();
  sdl_window = window;
}

CLibretro::~CLibretro()
{
  core_unload();
}

void CLibretro::core_changinpt(int dev, int port)
{
  if (lr_isrunning)
  {
    controller_type[port] = dev;
    retro.retro_set_controller_port_device(port, dev);
  }
}

bool no_roms2;
static bool no_roms(unsigned cmd, void *data)
{
  if (cmd == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME)
  {
    bool *bval = (bool *)data;
    no_roms2 = bval;
    return true;
  }
  return false;
}

bool CLibretro::core_load(char *ROM, bool game_specific_settings, char *corepath, bool contentless)
{
  if (lr_isrunning)
    core_unload();

  reset();

  int portage = 0;

  if (!core_inputbinds[0].size())
  {
    for (auto &controller : core_inputbinds)
    {
      for (int i = 0; i < 20; i++)
      {
        coreinput_bind bind;
        bind.isanalog = (i > 15);
        bind.retro_id = i;
        bind.config.bits.axistrigger = 0;
        bind.config.bits.sdl_id = 0;
        bind.config.bits.joytype = (uint8_t)joytype_::keyboard;
        bind.val = 0;
        bind.SDL_port = -1;
        bind.port = portage;
        bind.description = retro_descripts[i];
        bind.joykey_desc = "None";
        controller.push_back(bind);
      }
      portage++;
    }
  }

  std::filesystem::path romzpath = (ROM == NULL) ? "" : ROM;
  std::filesystem::path core_path_ = corepath;
  std::filesystem::path system_path_ = std::filesystem::path(exe_path) / "system";
  std::filesystem::path save_path_ = std::filesystem::path(exe_path) / "saves";
  std::filesystem::path save_path;

  if (contentless)
  {
    save_path = save_path_ / (core_path_.stem().string() + ".sram");
    rom_path = "";
  }
  else
  {
    rom_path = std::filesystem::absolute(romzpath).string();
    save_path = save_path_ / (romzpath.stem().string() + ".sram");
  }
  saves_path = std::filesystem::absolute(save_path_).string();
  system_path = std::filesystem::absolute(system_path_).string();
  romsavesstatespath = std::filesystem::absolute(save_path).string();

  if (game_specific_settings)
  {
    save_path = std::filesystem::absolute(save_path_).string();
    save_path.replace_filename(romzpath.stem().string() + ".corecfg");
  }
  else
  {
    save_path = std::filesystem::absolute(system_path_) /
                (core_path_.stem().string() + ".corecfg");
  }

  core_config = std::filesystem::absolute(save_path).string();

  void *hDLL = openlib((const char *)corepath);
  if (!hDLL)
  {
    const char *err = SDL_GetError();
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
  load_envsymb(retro.handle, true);
  retro.retro_init();
  load_envsymb(retro.handle, false);

  struct retro_game_info info = {0};
  struct retro_system_info system = {0};
  retro.retro_get_system_info(&system);
  retro_system_av_info av = {0};

  if (!contentless)
  {
    info = {ROM, 0};
    info.path = ROM;
    info.data = NULL;
    info.size = 0;
    info.meta = "";
    if (!system.need_fullpath)
    {
      info.size = get_filesize(ROM);

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
  }

  if (!retro.retro_load_game(contentless ? NULL : &info))
  {
    printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
    return false;
  }

  retro.retro_get_system_av_info(&av);
  SDL_DisplayMode dm;
  SDL_GetDesktopDisplayMode(0, &dm);
  refreshrate = dm.refresh_rate;
  audio_init((float)dm.refresh_rate, av.timing.sample_rate, av.timing.fps);
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
  static bool connotset = true;
  if (connotset)
  {
    core_changinpt(controller_type[0], 0);
    core_changinpt(controller_type[1], 1);
    connotset = false;
  }
}

void CLibretro::core_unload()
{
  if (lr_isrunning)
  {
    core_saveram(romsavesstatespath.c_str(), true);
    if (retro.handle != NULL)
    {
      audio_destroy();
      video_deinit();
      retro.retro_unload_game();
      retro.retro_deinit();
      freelib(retro.handle);
      retro.handle = NULL;
      memset((retro_core *)&retro, 0, sizeof(retro_core));
    }
    lr_isrunning = false;
    core_inputbinds[0].clear();
    core_inputbinds[1].clear();
    core_variables.clear();
    core_inputdesc[0].clear();
    core_inputdesc[1].clear();
  }
}

void CLibretro::get_cores()
{
  std::filesystem::path p(exe_path);
  std::filesystem::path path = p / "cores";
  for (auto &entry : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
  {
    string str = entry.path().string();
    if (entry.is_regular_file() && entry.path().extension() == SHLIB_EXTENSION)
    {

      struct retro_system_info system = {0};
      void *hDLL = openlib(str.c_str());
      if (!hDLL)
      {
        continue;
      }
      auto *getinfo = (void (*)(retro_system_info *))getfunc(hDLL, "retro_get_system_info");
      auto *set_environment =
          (void (*)(retro_environment_t))getfunc(hDLL, "retro_set_environment");
      no_roms2 = false;
      set_environment(no_roms);
      if (getinfo)
      {
        getinfo(&system);
        core_info entry_;
        entry_.fps = 60;
        entry_.samplerate = 44100;
        entry_.aspect_ratio = 4 / 3;
        entry_.core_name = system.library_name;
        entry_.core_extensions = (system.valid_extensions == NULL) ? "" : system.valid_extensions;
        entry_.core_path = str;
        entry_.no_roms = no_roms2;
        cores.push_back(entry_);
        freelib(hDLL);
      }
    }
  }

  coreexts = "";
  std::string corelist = "";
  for (auto &corez : cores)
  {
    if (!corez.no_roms)
    {
      std::stringstream test(corez.core_extensions);
      std::string segment;
      while (std::getline(test, segment, '|'))
        if (corelist.find(segment) == std::string::npos)
        {
          std::transform(segment.begin(), segment.end(), segment.begin(), ::tolower);
          corelist += segment + ",.";
        }
    }
  }
  if (corelist != "")
  {
    coreexts = "All supported {.";
    coreexts += corelist;
    coreexts.resize(coreexts.size() - 2);
    coreexts += "}";
  }
}