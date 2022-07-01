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

static void core_kb_callback(retro_keyboard_event_t e)
{
  inp_keys = e;
}

static bool core_controller_info(struct retro_controller_info *info)
{
  if (!info)
    return false;
  auto retro = CLibretro::get_classinstance();

  retro->core_inputdesc.clear();

  for (unsigned i = 0; i < info->num_types; i++)
  {
    const struct retro_controller_description d = info->types[i];
    if (d.desc)
    {
      coreinput_desc desc;
      desc.desc = d.desc;
      desc.id = d.id;
      retro->core_inputdesc.push_back(desc);
    }
  }
  return true;
}

static bool core_environment(unsigned cmd, void *data)
{
  bool *bval;
  auto retro = CLibretro::get_classinstance();

  switch (cmd)
  {

  case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
  {
    bool *bval = (bool *)data;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
  {
    struct retro_log_callback *cb = (struct retro_log_callback *)data;
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
  {
    return core_controller_info((struct retro_controller_info *)data);
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
    return video_sethw(hw);
  }

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval = (bool *)data;
    *bval = true;
    return true;
  case RETRO_ENVIRONMENT_GET_CORE_ASSETS_DIRECTORY:
  case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY: // 9
  {
    std::string str = retro->system_path;
    static char *sys_path = strdup((const char *)str.c_str());
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }
  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
  {
    std::string str = retro->saves_path;
    static char *sys_path = strdup((const char *)str.c_str());
    char **ppDir = (char **)data;
    *ppDir = sys_path;
    return true;
    break;
  }

  case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
  {
    const struct retro_keyboard_callback *callback = (const struct retro_keyboard_callback *)data;
    core_kb_callback(callback->callback);
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

  case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
    *(unsigned *)data = 1;
    return true;
    break;

  case RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL:
    return true;
    break;

  case RETRO_ENVIRONMENT_SET_MESSAGE:
  {
    const struct retro_message *msg = (const struct retro_message *)data;
    core_log(RETRO_LOG_DEBUG, "%s", msg);
    return true;
    break;
  }

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