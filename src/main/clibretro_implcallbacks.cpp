#include "clibretro.h"
#include <string>
#include <iostream>
#include <filesystem>
#include "io.h"
#include "utils.h"
#define INI_STRNICMP(s1, s2, cnt) (strcmp(s1, s2))
#include "ini.h"
#ifdef _WIN32
#include <windows.h>
#endif
using namespace std;

static void core_set_led_state(int led, int state)
{
}

static bool core_set_eject_state(bool ejected)
{
  auto lib = CLibretro::get_classinstance();
  return false;
}

static bool core_get_eject_state()
{
  auto lib = CLibretro::get_classinstance();
  return false;
}

static bool core_set_image_index(unsigned int index)
{
  auto lib = CLibretro::get_classinstance();
  return false;
}

static unsigned int core_get_image_index()
{
  auto lib = CLibretro::get_classinstance();
  return 0;
}

static unsigned int core_get_num_images()
{
  auto lib = CLibretro::get_classinstance();
  return 0;
}

static bool core_add_image_index()
{
  struct retro_disk disk;
  disk.path = "";
  disk.index = 0;

  auto lib = CLibretro::get_classinstance();
  lib->disk_intf.push_back(disk);
  return false;
}

static bool core_replace_image_index(unsigned int index, const retro_game_info *info)
{
  auto lib = CLibretro::get_classinstance();
  return false;
}

static void core_audio_sample(int16_t left, int16_t right)
{
  auto lib = CLibretro::get_classinstance();
  if (lib->core_isrunning())
  {
    int16_t buf[2] = {left, right};
    audio_mix(buf, 1);
  }
}
static size_t core_audio_sample_batch(const int16_t *data, size_t frames)
{
  if (!frames && data == NULL)
    return 0;
  auto lib = CLibretro::get_classinstance();
  if (lib->core_isrunning())
  {
    audio_mix(data, frames);
  }
  return frames;
}

static void core_log(enum retro_log_level level, const char *fmt, ...)
{
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

static void core_kb_callback(retro_keyboard_event_t e)
{
  inp_keys = e;
}

static bool core_controller_info(struct retro_controller_info *info)
{
  if (!info)
    return false;
  auto retro = CLibretro::get_classinstance();
  retro->core_inputdesc[0].clear();
  retro->core_inputdesc[1].clear();
  bool cont_found = false;
  struct retro_controller_info *info2 = info;
  for (int j = 0; j < 2; j++)
  {
    if (info2 == NULL)
      break;
    for (unsigned i = 0; i < info2->num_types; i++)
    {
      if (info2 == NULL)
        break;
      const struct retro_controller_description d = info2->types[i];
      if (d.desc)
      {
        if (d.id == RETRO_DEVICE_JOYPAD && !cont_found)
        {
          cont_found = true;
          retro->controller_type[j] = d.id;
        }
        coreinput_desc desc;
        desc.desc = d.desc;
        desc.id = d.id;
        retro->core_inputdesc[j].push_back(desc);
      }
    }
    info2++;
  }
  return true;
}

static uint64_t core_get_cpu_features()
{

  std::tuple<int, SDL_bool (*)()> simd_masks[] = {
      {RETRO_SIMD_AVX, SDL_HasAVX},
      {RETRO_SIMD_AVX2, SDL_HasAVX2},
      {RETRO_SIMD_MMX, SDL_HasMMX},
      {RETRO_SIMD_SSE, SDL_HasSSE},
      {RETRO_SIMD_SSE2, SDL_HasSSE2},
      {RETRO_SIMD_SSE3, SDL_HasSSE3},
      {RETRO_SIMD_SSE4, SDL_HasSSE41},
      {RETRO_SIMD_SSE42, SDL_HasSSE42},
  };
  uint64_t cpu = 0;
  for (auto [rarch_mask, vecfunc] : simd_masks)
  {
    if (vecfunc())
      cpu |= rarch_mask;
  }
  return cpu;
}

retro_time_t cpu_features_get_time_usec(void)
{
  return (retro_time_t)SDL_GetTicks() * 1000;
}

static retro_perf_tick_t core_get_perf_counter()
{
  return (retro_perf_tick_t)SDL_GetPerformanceCounter();
}

static void core_perf_register(struct retro_perf_counter *counter)
{
  auto retro = CLibretro::get_classinstance();
  retro->perf_counter_last = counter;
  counter->registered = true;
}

static void core_perf_log()
{
}

static void core_perf_start(struct retro_perf_counter *counter)
{
  if (counter->registered)
  {
    counter->start = core_get_perf_counter();
  }
}

static void core_perf_stop(struct retro_perf_counter *counter)
{
  counter->total = core_get_perf_counter() - counter->start;
}

static bool core_environment(unsigned cmd, void *data)
{
  bool *bval;
  auto retro = CLibretro::get_classinstance();
  switch (cmd)
  {

  case RETRO_ENVIRONMENT_GET_PERF_INTERFACE:
  {
    struct retro_perf_callback *perf = (struct retro_perf_callback *)data;
    perf->get_time_usec = cpu_features_get_time_usec;
    perf->get_cpu_features = core_get_cpu_features;
    perf->get_perf_counter = core_get_perf_counter;
    perf->perf_register = core_perf_register;
    perf->perf_start = core_perf_start;
    perf->perf_stop = core_perf_stop;
    perf->perf_log = core_perf_log;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_ROTATION:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_OVERSCAN:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_RUMBLE_INTERFACE:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_SENSOR_INTERFACE:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_CAMERA_INTERFACE:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_LOCATION_INTERFACE:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
  {
    auto *var = (struct retro_led_interface *)data;
    var->set_led_state = core_set_led_state;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
  {
    auto *var = (const struct retro_disk_control_callback *)data;
    return false;
  }

  case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
  {
    auto *bval = (bool *)data;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_CURRENT_SOFTWARE_FRAMEBUFFER:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS:
  {
    auto *players = (unsigned *)data;
    *players = 2;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_MEMORY_MAPS:
  {
    return false;
  }

  case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
  {
    auto *capabilities = (uint64_t *)data;
    *capabilities = (1 << RETRO_DEVICE_JOYPAD) |
                    (1 << RETRO_DEVICE_MOUSE) |
                    (1 << RETRO_DEVICE_KEYBOARD) |
                    (1 << RETRO_DEVICE_POINTER);
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
  {
    auto *cb = (struct retro_log_callback *)data;
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LANGUAGE:
  {
    *(unsigned *)data = RETRO_LANGUAGE_ENGLISH;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
  {
    *(bool *)data = false;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_USERNAME:
  {
    *(const char **)data = "mudlord";
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
  {
    return core_controller_info((struct retro_controller_info *)data);
  }

  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
  {
    auto info = (struct retro_system_av_info *)data;
    auto *geo = (struct retro_game_geometry *)&info->geometry;
    video_changegeom(geo);
    audio_changeratefps(retro->refreshrate, info->timing.sample_rate, info->timing.fps);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_GEOMETRY:
  {
    auto *geom = (struct retro_game_geometry *)data;
    video_changegeom(geom);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
  {
    auto *cb = (struct retro_core_option_display *)data;
    for (size_t i = 0; i < retro->core_variables.size(); i++)
    {
      if (strcmp(retro->core_variables[i].name.c_str(), cb->key) == 0)
      {
        retro->core_variables[i].config_visible = cb->visible;
        return true;
      }
    }
    return false;
    break;
  }

  case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
  {
    *(unsigned *)data = RETRO_HW_CONTEXT_OPENGL;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_HW_RENDER:
  {
    auto *hw =
        (struct retro_hw_render_callback *)data;
    return video_sethw(hw);
  }

  case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT:
  {
    return true;
  }

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval = (bool *)data;
    *bval = true;
    return true;

  case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
  {
    const char **dir = (const char **)data;
    std::filesystem::path p(get_wtfwegname());
    static auto *lr_path = strdup(p.parent_path().string().c_str());
    *dir = lr_path;
    return true;
  }
  case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
  {
    std::string str = retro->system_path;
    static auto *sys_path = strdup(str.c_str());
    auto **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }
  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
  {
    std::string str = retro->saves_path;
    static auto *sys_path = strdup(str.c_str());
    auto **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
  {
    auto *callback = (const struct retro_keyboard_callback *)data;
    core_kb_callback(callback->callback);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 31
  {
    auto *var = (struct retro_input_descriptor *)data;
    retro->init_inputvars(var);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_VARIABLES:
  {
    auto *vars = (const struct retro_variable *)data;
    return retro->init_configvars((retro_variable *)vars);
  }

  case RETRO_ENVIRONMENT_GET_VARIABLE:
  {
    auto *var = (struct retro_variable *)data;
    var->value = retro->load_corevars(var);
    return true;
  }
  break;
  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
  {
    *(bool *)data = retro->variables_changed;
    retro->variables_changed = false;
    return true;
  }
  break;

  case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
    *(unsigned *)data = 1;
    return true;
    break;

  case RETRO_ENVIRONMENT_SET_MESSAGE:
  {
    auto *msg = (const struct retro_message *)data;
    core_log(RETRO_LOG_DEBUG, "%s", msg);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
  {
    auto *fmt = (enum retro_pixel_format *)data;
    return video_set_pixelformat(*fmt);
  }
  default:
    core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
    return false;
  }
  return false;
}

static void core_video_refresh(const void *data, unsigned width,
                               unsigned height, size_t pitch)
{
  auto retro = CLibretro::get_classinstance();
  if (retro->core_isrunning())
    video_refresh(data, width, height, pitch);
}

static void core_input_poll(void)
{
  poll_lr();
}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index,
                                unsigned id)
{
  return input_state(port, device, index,
                     id);
}

void CLibretro::load_envsymb(void *handle, bool first)
{
#define libload(name) SDL_LoadFunction(handle, name)
#define load_sym(V, name) if (!(*(void **)(&V) = (void *)libload(#name)))

  if (first)
  {
    void (*set_environment)(retro_environment_t) = NULL;
    load_sym(set_environment, retro_set_environment);
    set_environment(core_environment);
  }
  else
  {
    void (*set_video_refresh)(retro_video_refresh_t) = NULL;
    void (*set_input_poll)(retro_input_poll_t) = NULL;
    void (*set_input_state)(retro_input_state_t) = NULL;
    void (*set_audio_sample)(retro_audio_sample_t) = NULL;
    void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
    load_sym(set_video_refresh, retro_set_video_refresh);
    load_sym(set_input_poll, retro_set_input_poll);
    load_sym(set_input_state, retro_set_input_state);
    load_sym(set_audio_sample, retro_set_audio_sample);
    load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);
    set_video_refresh(core_video_refresh);
    set_input_poll(core_input_poll);
    set_input_state(core_input_state);
    set_audio_sample(core_audio_sample);
    set_audio_sample_batch(core_audio_sample_batch);
  }
}