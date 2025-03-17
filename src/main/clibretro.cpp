#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "inout.h"
#include "out_aud.h"
#include <algorithm>
#include <array>

#ifdef _WIN32
#include <windows.h>
#include "MemoryModulePP.h"
static std::string_view SHLIB_EXTENSION = ".dll";
#else
static std::string_view SHLIB_EXTENSION = ".so";
#endif

#include "cJSON.h"
#include "cJSON_Utils.h"

#include "mudutils/utils.h"
#include "unistd.h"
using namespace std;

bool CLibretro::core_savestateslot(bool save)
{
  if (rom_path != "")
  {
    std::filesystem::path save_path = std::filesystem::path(exe_path) / "saves" /
                                      (rom_path.stem().string() + "_" + std::to_string(save_slot) + ".sram");
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
    auto Memory = std::make_unique<uint8_t[]>(size);
    if (!size || !Memory)
      return false;
    if (save)
    {
      // Get the filesize
      retro.retro_serialize(Memory.get(), size);
      MudUtil::save_data(Memory.get(), size, filename);
    }
    else
    {
      std::vector<uint8_t> save_ptr = MudUtil::load_data(filename);
      if (save_ptr.empty())
        return false;
      memcpy(Memory.get(), save_ptr.data(), save_ptr.size());
      retro.retro_unserialize(Memory.get(), save_ptr.size());
    }
    return true;
  }
  return false;
}

bool CLibretro::core_saveram(const char *filename, bool save)
{
  if (lr_isrunning)
  {
    size_t size = retro.retro_get_memory_size((usesubsys) ? subsystem_id : RETRO_MEMORY_SAVE_RAM);
    uint8_t *Memory = (uint8_t *)retro.retro_get_memory_data((usesubsys) ? subsystem_id : RETRO_MEMORY_SAVE_RAM);
    if (!size || !Memory)
      return false;
    if (save)
      return MudUtil::save_data(Memory, size, filename);
    else
    {
      std::vector<uint8_t> save_ptr = MudUtil::load_data(filename);
      if (save_ptr.empty() || save_ptr.size() != size)
        return false;
      memcpy(Memory, save_ptr.data(), save_ptr.size());
      return true;
    }
  }
  return false;
}

bool CLibretro::load_coresettings(bool save_f)
{
  int size_ = MudUtil::get_filesize(core_config.c_str());
  std::vector<uint8_t> data;
  cJSON *ini = NULL;
  if (size_)
  {
    data = MudUtil::load_data(core_config.c_str());
    ini = cJSON_Parse((char *)data.data());
  }
  else
  {
    save_f = true;
    ini = cJSON_CreateObject();
  }

  cJSON *config = NULL;
  cJSON *config_entries = NULL;
  if (cJSON_HasObjectItem(ini, std::to_string(config_crc).c_str()))
  {
    config = cJSON_GetObjectItemCaseSensitive(ini, std::to_string(config_crc).c_str());
    config_entries = cJSON_GetArrayItem(config, 0);
  }
  else
  {
    save_f = true;
    config = cJSON_AddArrayToObject(ini, std::to_string(config_crc).c_str());
    config_entries = cJSON_CreateObject();
    cJSON_AddItemToArray(config, config_entries);
  }

  for (auto &vars : core_variables)
  {
    {
      if (cJSON_HasObjectItem(config_entries, vars.name.c_str()))
      {
        cJSON *configval = cJSON_GetObjectItemCaseSensitive(config_entries, vars.name.c_str());
        if (save_f)
          cJSON_SetValuestring(configval, vars.var.c_str());
        else
          vars.var = cJSON_GetStringValue(configval);
      }
      else
      {
        cJSON_AddStringToObject(config_entries, vars.name.c_str(), vars.var.c_str());
        save_f = true;
      }
    }
    for (auto j = std::size_t{}; auto &var_val : vars.config_vals)
    {
      if (var_val == vars.var)
      {
        vars.sel_idx = j;
        break;
      }
      j++;
    }
  }
  if (save_f)
  {
    std::string json = cJSON_Print(ini);
    MudUtil::save_data((unsigned char *)json.c_str(), json.length(), core_config.c_str());
  }
  cJSON_Delete(ini);
  return true;
}

bool CLibretro::init_inputvars(retro_input_descriptor *var)
{
  load_inpcfg(var);
  return true;
}

const char *CLibretro::load_corevars(retro_variable *var)
{
  for (auto &vars : core_variables)
    if (strcmp(vars.name.c_str(), var->key) == 0)
      return vars.var.c_str();
  return NULL;
}

bool CLibretro::init_configvars_coreoptions(void *var, int version)
{
  core_variables.clear();
  core_categories.clear();
  variables_changed = false;

  if (version > 1)
  {
    auto *var3 = reinterpret_cast<struct retro_core_options_v2 *>(var);
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
      vars_struct.config_visible = true;
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
    auto *var3 = reinterpret_cast<struct retro_core_option_definition *>(var);
    while (var3->key != NULL)
    {
      retro_core_option_value *var1 = var3->values;
      loadedcore_configvars vars_struct;
      vars_struct.sel_idx = 0;
      if(var3->default_value)
      vars_struct.var = var3->default_value;
      else
      vars_struct.var =var1->value;
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
  load_coresettings(false);
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

  load_coresettings(false);
  v2_vars = false;
  return true;
}

/*
libretro "retropad" arrangement:

    L3                     R3
    L2                     R2
    L                      R

    up                     X
 left right select start  Y A
    down                   B

      leftstick  rightstick

l2-3/r2-3 can be analog buttons as well as digital
rest are purely digital except for sticks
*/

void CLibretro::reset()
{
  core_config = (std::filesystem::path(exe_path) / "wtfweg.json").string();
  inp_keys = NULL;
  lr_isrunning = false;
  save_slot = 0;
  variables_changed = false;
  perf_counter_last = 0;
  frametime_cb = NULL;
  frametime_ref = 0;
  use_retropad = true;
  fps = 60.0;
  perfc = SDL_GetPerformanceFrequency();
  load_savestate = "";

  reset_retropad();

  disk_intf.clear();
  core_variables.clear();
  v2_vars = false;
  memset(&retro, 0, sizeof(retro));

  input_confcrc = 0;
  for (auto &controller : core_inpbinds)
    for (auto &bind : controller.inputbinds)
      input_confcrc = MudUtil::crc32(input_confcrc, bind.description.c_str(), bind.description.length());
  loadinpconf(input_confcrc, false);
}

void CLibretro::init_lr(SDL_Window *window)
{
  std::filesystem::path exe(MudUtil::get_wtfwegname());
  exe_path = exe.parent_path().string();
  std::array<std::string, 3> dirs = {"cores", "system", "saves"};
  for (auto &dir : dirs)
  {
    std::filesystem::path p = exe.parent_path().string();
    std::filesystem::path path = p / dir;
    std::filesystem::create_directory(path);
  }

  reset();

  cores.clear();

  get_cores();
  sdl_window = window;
}

void CLibretro::core_changinpt(int dev, int port)
{
  if (lr_isrunning)
  {
    if (core_inpbinds[port].controller_type != dev)
      core_inpbinds[port].controller_type = dev;
    retro.retro_set_controller_port_device(port, dev);
  }
}

bool CLibretro::core_load(clibretro_startoptions *options)
{
  if (lr_isrunning)
    core_unload();

  reset();
  lr_isrunning = false;

  rom_path = options->rompaths[0];
  std::filesystem::path path_core(options->core);
  std::filesystem::path save_path_ = std::filesystem::path(exe_path) / "saves";
  std::filesystem::path save_path = save_path_ /
                                    (std::filesystem::path(options->core).stem().string() + ".sram");
  if (!options->contentless)
    save_path = save_path_ / (rom_path.stem().string() + ".sram");

  romsavesstatespath = std::filesystem::absolute(save_path).string();

  if (options->game_specific_settings)
  {
    save_path = std::filesystem::absolute(save_path_).string();
    save_path.replace_filename(rom_path.stem().string() + ".corecfg");
    core_config = std::filesystem::absolute(save_path).string();
  }

  void *hDLL = NULL;
  hDLL = MudUtil::openlib((const char *)options->core.c_str());
  if (!hDLL)
  {
    const char *err = SDL_GetError();
    return false;
  }
  config_crc = MudUtil::crc32(0, path_core.filename().string().c_str(),
                              path_core.filename().string().length());

#define libload(name) MudUtil::getfunc(hDLL, name)
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

  struct retro_system_info system = {0};
  retro.retro_get_system_info(&system);
  retro_system_av_info av = {0};

  usesubsys = options->usesubsys;
  if (!options->usesubsys)
  {
    struct retro_game_info info = {options->rompaths[0].c_str(), 0};
    if (!options->contentless)
    {
      info.path = options->rompaths[0].c_str();
      info.size = MudUtil::get_filesize(options->rompaths[0].c_str());
      info.meta = "";
      info.data = NULL;
      if (!system.need_fullpath)
      {
        std::ifstream ifs(options->rompaths[0], ios::binary);
        if (!ifs.good())
        {
        fail:
          printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
          ifs.close();
          core_unload();
          return false;
        }
        info.data = malloc(info.size);
        if (!info.data)
          goto fail;

        ifs.read((char *)info.data, info.size);
        ifs.close();
      }
    }
    if (!retro.retro_load_game(options->contentless ? NULL : &info))
    {
      printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
      core_unload();
      return false;
    }
  }
  else
  {
    std::vector<retro_game_info>info_games;
    info_games.clear();
    //int info_size = options->rompaths.size();
  //  retro_game_info *info = (retro_game_info *)malloc(sizeof(retro_game_info *) * info_size);
    subsystem_id = options->current_subsystem.subsystem_id;
    for (auto &romz : options->rompaths)
    {
      retro_game_info info = {0};
      size_t r = &romz - &options->rompaths.front();
      subsystem_rominfo rominfo = options->current_subsystem.rominfo[r];
      info.data = NULL;
      info.size = MudUtil::get_filesize(romz.c_str());
      info.meta = "";
      if (rominfo.required)
      {
        if (rominfo.need_fullpath)
          info.path = romz.c_str();
        else
        {
          std::ifstream ifs(romz, ios::binary);
          if (!ifs.good())
          {
            printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
            ifs.close();
            core_unload();
            return false;
          }
          info.data = malloc(info.size);
          if (!info.data)
          {
            printf("FAILED TO LOAD ROMz!!!!!!!!!!!!!!!!!!");
            ifs.close();
            core_unload();
            return false;
          }
          ifs.read((char *)info.data, info.size);
          ifs.close();
        }
      }
      info_games.push_back(info);
    }
    if (!retro.retro_load_game_special(subsystem_id, info_games.data(), info_games.size()))
    {
      printf("FAILED TO LOAD ROM!!!!!!!!!!!!!!!!!!");
      core_unload();
      return false;
    }
    
  }
  retro.retro_get_system_av_info(&av);
  fps = perfc * 1000 / uint64_t(1000.0 * std::abs(av.timing.fps));
  audio.init(av.timing.sample_rate);
  audio.framelimit(options->framelimit);

  core_fps = av.timing.fps;
  core_samplerate = av.timing.sample_rate;

  video.init(&av.geometry);

  loadcontconfig(false);

  lr_isrunning = true;
  for (int i = 0; i < core_inpbinds.size(); i++)
    core_changinpt(core_inpbinds[i].controller_type, i);
  core_saveram(romsavesstatespath.c_str(), false);

  load_savestate = options->savestate;
  framecap(options->framelimit);
  frameno = 0;

  min_deltime = 1000. / core_fps;
  max_deltatime = min_deltime;

  return true;
}

void CLibretro::core_framelimit()
{
  if (capfps)
  {
    auto supersleep = [](uint64_t target, uint64_t perf)
    {
      uint64_t time = SDL_GetPerformanceCounter();
      if (time >= target)
      {
        return;
      }
      // Because OS sleep is not accurate,
      // we actually sleep until a maximum of 2 milliseconds are left.
      while (int64_t(target - time) * 1000 > 2 * int64_t(perf))
      {
        SDL_Delay(1);
        time = SDL_GetPerformanceCounter();
      }
      int64_t remain;
      do
      {
#ifdef SDL_CPUPauseInstruction
        SDL_CPUPauseInstruction();
#endif
        remain = target - SDL_GetPerformanceCounter();
      } while (remain > 0);
    };

    static uint64_t time = 0;
    supersleep(time, perfc);

    time = SDL_GetPerformanceCounter() + fps;
  }
}

bool CLibretro::core_isrunning()
{
  return lr_isrunning;
}

void CLibretro::core_run()
{
  if (frametime_cb != NULL)
    frametime_cb(frametime_ref);
  uint64_t now = SDL_GetPerformanceCounter();
  retro.retro_run();
  uint64_t last = SDL_GetPerformanceCounter();
  deltatime = (double)((last - now) * 1000 / (double)perfc);

  if (frames.size() > 200) // Max seconds to show
  {
    for (size_t i = 1; i < frames.size(); i++)
    {
      frames[i - 1] = frames[i];
    }
    frames[frames.size() - 1] = deltatime;
  }
  else
  {
    frames.push_back(deltatime);
  }

  frameno++;

  if (load_savestate != "")
  {
    static bool loaded = false;
    if (!loaded)
      core_savestate(load_savestate.c_str(), false);
    loaded = true;
  }
}

void CLibretro::core_unload()
{
  if (lr_isrunning)
  {
    core_saveram(romsavesstatespath.c_str(), true);
    if (retro.handle != NULL)
    {
      audio.destroy();
      video.destroy();
      retro.retro_unload_game();
      retro.retro_deinit();
      MudUtil::freelib(retro.handle);
      retro.handle = NULL;
      memset((retro_core *)&retro, 0, sizeof(retro_core));
    }
    lr_isrunning = false;
    core_inpbinds.clear();
    core_inputttypes.clear();
    core_variables.clear();
    reset();
  }
}

void CLibretro::get_cores()
{
  std::filesystem::path p(exe_path);
  coreexts = "";
  std::string corelist = "";

  std::filesystem::path path = p / "cores";
  for (auto &entry : std::filesystem::directory_iterator(path, std::filesystem::directory_options::skip_permission_denied))
  {
    string str = entry.path().string();
    if (entry.is_regular_file() && (entry.path().extension() == SHLIB_EXTENSION
#ifdef _WIN32
#ifndef DEBUG
                                    || entry.path().extension() == ".zip"
#endif
#endif
                                    ))
    {
      struct retro_system_info system = {0};
      void *hDLL = MudUtil::openlib(str.c_str());
      if (!hDLL)
      {
        continue;
      }
      static bool no_roms2;
      no_roms2 = false;
      static std::vector<subsystems> subsys;
      subsys.clear();
      auto no_roms = +[](unsigned cmd, void *data) -> bool
      {
        if (cmd == RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME)
        {
          bool *bval = reinterpret_cast<bool *>(data);
          no_roms2 = bval;
          return true;
        }
        if (cmd == RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO)
        {
          retro_subsystem_info *subs = (retro_subsystem_info *)data;
          while (subs->ident != NULL)
          {
            subsystems sub = {};
            sub.subsystem_id = subs->id;
            sub.subsystem_name = subs->desc;

            retro_subsystem_rom_info *roms = (retro_subsystem_rom_info *)subs->roms;
            for (int i = 0; i < subs->num_roms; i++)
            {
              subsystem_rominfo rominfo = {};
              if (roms->memory != NULL)
              {
                rominfo.memory_ext = roms->memory->extension;
                rominfo.memory_id = roms->memory->type;
              }

              rominfo.block_extract = roms->block_extract;
              rominfo.need_fullpath = roms->need_fullpath;
              rominfo.romexts = roms->valid_extensions;
              rominfo.required = roms->required;
              rominfo.romtype = roms->desc;
              sub.rominfo.push_back(rominfo);
              roms++;
            }
            subsys.push_back(sub);
            subs++;
          }

          return true;
        }
        return false;
      };

      auto *getinfo = (void (*)(retro_system_info *))MudUtil::getfunc(hDLL, "retro_get_system_info");
      auto *set_environment =
          (void (*)(retro_environment_t))MudUtil::getfunc(hDLL, "retro_set_environment");
      set_environment(no_roms);
      if (getinfo)
      {
        getinfo(&system);
        core_info entry_;
        entry_.fps = 60;
        entry_.subsystems = subsys;
        entry_.samplerate = 44100;
        entry_.aspect_ratio = 4 / 3;
        entry_.core_name = system.library_name;
        entry_.core_extensions = (system.valid_extensions == NULL) ? "" : system.valid_extensions;
        entry_.core_path = str;
        entry_.no_roms = no_roms2;
        if (entry_.core_extensions != "")
        {
          std::string test = entry_.core_extensions;
          test = MudUtil::replace_all(test, "|", ",.");
          corelist += test + ",.";
        }
        cores.push_back(entry_);
        MudUtil::freelib(hDLL);
      }
    }
  }
  std::string sep = ",.";
  for (auto &corez : cores)
  {
    if (!corez.no_roms)
      corelist += MudUtil::replace_all(corez.core_extensions, "|", sep) + sep;
  }
  if (corelist != "")
  {
    coreexts = "All supported {.";
    corelist = corelist.substr(0, corelist.size() - sep.size());
    coreexts += corelist;
    coreexts += "}";
  }
}