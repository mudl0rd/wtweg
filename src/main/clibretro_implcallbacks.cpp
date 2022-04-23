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

static bool core_environment(unsigned cmd, void *data)
{
  bool *bval;
  auto retro = CLibretro::get_classinstance();

  switch (cmd)
  {
  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
  {
    struct retro_log_callback *cb = (struct retro_log_callback *)data;
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
  {
    struct retro_core_option_display *cb = (struct retro_core_option_display *)data;
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

  case RETRO_ENVIRONMENT_SET_HW_RENDER:
  {
    struct retro_hw_render_callback *hw =
        (struct retro_hw_render_callback *)data;
    if (hw->context_type == RETRO_HW_CONTEXT_VULKAN)
      return false;
    return video_sethw(hw);
  }

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval = (bool *)data;
    *bval = true;
    return true;
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
  {
    std::string str = retro->saves_path;
    static char *sys_path = strdup((const char*)str.c_str());
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }

  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 31
  {
    struct retro_input_descriptor *var = (struct retro_input_descriptor *)data;
    retro->init_inputvars(var);
    return true;
  }

  break;

  case RETRO_ENVIRONMENT_SET_VARIABLES:
  {
    const struct retro_variable *vars = (const struct retro_variable *)data;
    return retro->init_configvars((retro_variable *)vars);
  }
  break;

  case RETRO_ENVIRONMENT_GET_VARIABLE:
  {
    struct retro_variable *var = (struct retro_variable *)data;

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

  case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT:
  {
    enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;
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
  if(retro->core_isrunning())
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

void CLibretro::load_envsymb(void *handle)
{
#define libload(name) SDL_LoadFunction(handle, name)
#define load_sym(V, name) if (!(*(void **)(&V) = (void *)libload(#name)))
  void (*set_environment)(retro_environment_t) = NULL;
  void (*set_video_refresh)(retro_video_refresh_t) = NULL;
  void (*set_input_poll)(retro_input_poll_t) = NULL;
  void (*set_input_state)(retro_input_state_t) = NULL;
  void (*set_audio_sample)(retro_audio_sample_t) = NULL;
  void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;
  load_sym(set_environment, retro_set_environment);
  load_sym(set_video_refresh, retro_set_video_refresh);
  load_sym(set_input_poll, retro_set_input_poll);
  load_sym(set_input_state, retro_set_input_state);
  load_sym(set_audio_sample, retro_set_audio_sample);
  load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);
  set_environment(core_environment);
  set_video_refresh(core_video_refresh);
  set_input_poll(core_input_poll);
  set_input_state(core_input_state);
  set_audio_sample(core_audio_sample);
  set_audio_sample_batch(core_audio_sample_batch);
}