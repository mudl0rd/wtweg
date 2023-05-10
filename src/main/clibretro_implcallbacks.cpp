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

struct retro_vfs_file_handle
{
  std::string curr_file;
  std::fstream file_ptr;
};

const char *vfs_file_path(retro_vfs_file_handle *handle)
{
  return handle->curr_file.c_str();
}

retro_vfs_file_handle *vfs_open(const char *path, unsigned mode, unsigned hints)
{
  retro_vfs_file_handle *p = new retro_vfs_file_handle();
  // https://github.com/jermp/mm_file
  if (strstr(path, "cdrom://"))
  {
    // add in CD drive emulation.
    // afaik the core shouldnt know whats this file handlde anyway, so it could be anything
    // though reading cores that actually implement vfs makes it tonz less clear.
    return NULL;
  }
  switch (mode)
  {
  case RETRO_VFS_FILE_ACCESS_READ:
    p->file_ptr.open(path, std::fstream::in);
    break;
  case RETRO_VFS_FILE_ACCESS_READ_WRITE:
    p->file_ptr.open(path, std::fstream::in | std::fstream::out);
    break;
  case RETRO_VFS_FILE_ACCESS_WRITE:
    p->file_ptr.open(path, std::fstream::out);
    break;
  case RETRO_VFS_FILE_ACCESS_UPDATE_EXISTING:
    p->file_ptr.open(path, std::fstream::in | std::fstream::out | std::fstream::app);
  }
  if (!p->file_ptr.is_open())
  {
    delete p;
    return NULL;
  }
  p->curr_file = path;
  return p;
}

int vfs_close(struct retro_vfs_file_handle *stream)
{
  if (stream)
  {
    stream->file_ptr.close();
    delete stream;
    return 1;
  }
  return -1;
}

int64_t vfs_size(struct retro_vfs_file_handle *stream)
{
  return (stream) ? std::filesystem::file_size(stream->curr_file) : -1;
}

int64_t vfs_tell(struct retro_vfs_file_handle *stream)
{
  return (stream) ? (int64_t)stream->file_ptr.tellg() : -1;
}

int64_t vfs_seek(struct retro_vfs_file_handle *stream, int64_t offset, int seek_position)
{
  if (stream != NULL)
  {
    switch (seek_position)
    {
    case RETRO_VFS_SEEK_POSITION_START:
      stream->file_ptr.seekg(offset, ios::beg);
      break;
    case RETRO_VFS_SEEK_POSITION_CURRENT:
      stream->file_ptr.seekg(offset, ios::cur);
      break;
    case RETRO_VFS_SEEK_POSITION_END:
      stream->file_ptr.seekg(offset, ios::end);
      break;
    }
    return stream->file_ptr.tellg();
  }
  return -1;
}

int64_t vfs_read(struct retro_vfs_file_handle *stream, void *s, uint64_t len)
{
  if (stream != NULL)
  {
    stream->file_ptr.read(reinterpret_cast<char *>(s), len);
    return stream->file_ptr.tellg();
  }
  return -1;
}

int64_t vfs_write(struct retro_vfs_file_handle *stream, const void *s, uint64_t len)
{
  if (stream != NULL)
  {
    stream->file_ptr.write(reinterpret_cast<const char *>(s), len);
    return stream->file_ptr.tellg();
  }
  return -1;
}

int vfs_flush(struct retro_vfs_file_handle *stream)
{
  if (stream != NULL)
  {
    stream->file_ptr.flush();
    return 0;
  }
  return -1;
}

int vfs_remove(const char *path)
{
  return std::remove(path);
}

int vfs_rename(const char *old_path, const char *new_path)
{
  std::filesystem::rename(old_path, new_path);
  return 0;
}

int64_t vfs_truncate(struct retro_vfs_file_handle *stream, int64_t length)
{
  std::filesystem::resize_file(stream->curr_file, length);
  return length;
}

retro_vfs_interface vfs_intf{
    /* VFS API v1 */
    vfs_file_path,
    vfs_open,
    vfs_close,
    vfs_size,
    vfs_tell,
    vfs_seek,
    vfs_read,
    vfs_write,
    vfs_flush,
    vfs_remove,
    vfs_rename,
    vfs_truncate,
    /* VFS API v3 */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL};

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
  add_log(level, buffer);
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
  const std::tuple<int, SDL_bool (*)()> simd_masks[] = {
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
    struct retro_perf_callback *perf = reinterpret_cast<struct retro_perf_callback *>(data);
    perf->get_time_usec = cpu_features_get_time_usec;
    perf->get_cpu_features = core_get_cpu_features;
    perf->get_perf_counter = core_get_perf_counter;
    perf->perf_register = core_perf_register;
    perf->perf_start = core_perf_start;
    perf->perf_stop = core_perf_stop;
    perf->perf_log = core_perf_log;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LED_INTERFACE:
  {
    auto *var = reinterpret_cast<struct retro_led_interface  *>(data);;
    var->set_led_state = core_set_led_state;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE:
  {
    auto *var = reinterpret_cast<const struct retro_disk_control_callback *>(data);
    return false;
  }

  case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME:
  {
    return true;
  }

  case RETRO_ENVIRONMENT_SET_SUBSYSTEM_INFO:
  {
    // the more and more I read example code using this, the less it makes sense to
    // implement it.
    return false;
  }

  case RETRO_ENVIRONMENT_GET_INPUT_MAX_USERS:
  {
    auto *players =  reinterpret_cast<unsigned *>(data);
    *players = 2;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_INPUT_DEVICE_CAPABILITIES:
  {
    auto *capabilities =  reinterpret_cast<uint64_t *>(data);
    *capabilities = (1 << RETRO_DEVICE_JOYPAD) |
                    (1 << RETRO_DEVICE_MOUSE) |
                    (1 << RETRO_DEVICE_KEYBOARD) |
                    (1 << RETRO_DEVICE_POINTER);
    return true;
  }

  case RETRO_ENVIRONMENT_GET_VFS_INTERFACE:
  {
   struct retro_vfs_interface_info *cb = reinterpret_cast<struct retro_vfs_interface_info*>(data);
    if (cb->required_interface_version > 2)
      return false;
    struct retro_vfs_interface *vfs_iface = (struct retro_vfs_interface*)&vfs_intf;
    cb->iface = reinterpret_cast<struct retro_vfs_interface *>(vfs_iface);
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LOG_INTERFACE:
  {
    auto *cb = reinterpret_cast<struct retro_log_callback*>(data);
    cb->log = core_log;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_LANGUAGE:
  {
    unsigned *dat= reinterpret_cast<unsigned*>(data);
    *dat = RETRO_LANGUAGE_ENGLISH;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_FASTFORWARDING:
  {
    bool* dat = reinterpret_cast<bool*>(data);
    *dat = false;
    return true;
  }

  case RETRO_ENVIRONMENT_GET_USERNAME:
  {
    const char** dat = reinterpret_cast<const char **>(data);
    *dat = "mudlord";
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CONTROLLER_INFO:
  {
    return core_controller_info(reinterpret_cast<struct retro_controller_info *>(data));
  }

  case RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO:
  {
    auto info = reinterpret_cast<struct retro_system_av_info *>(data);
    auto *geo = (struct retro_game_geometry *)&info->geometry;
    video_changegeom(geo);
    audio_changeratefps(retro->refreshrate, info->timing.sample_rate, info->timing.fps);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_GEOMETRY:
  {
    auto *geom = reinterpret_cast<struct retro_game_geometry *>(data);
    video_changegeom(geom);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY:
  {
    auto *cb = reinterpret_cast<struct retro_core_option_display *>(data);

    for (auto &var : retro->core_variables)
    {
      if (strcmp(var.name.c_str(), cb->key) == 0)
      {
        var.config_visible = cb->visible;
        for (auto &var2 : retro->core_categories)
          if ((var.category_name == var2.key))
            var2.visible = cb->visible;
        return true;
      }
    }
    return false;
  }

  case RETRO_ENVIRONMENT_GET_PREFERRED_HW_RENDER:
  {
    unsigned * dat = reinterpret_cast<unsigned *>(data);
  *dat = RETRO_HW_CONTEXT_OPENGL;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_HW_RENDER:
  {
    auto *hw =
    reinterpret_cast<struct retro_hw_render_callback *>(data);
    return video_sethw(hw);
  }

  case RETRO_ENVIRONMENT_SET_HW_SHARED_CONTEXT:
  {
    return true;
  }

  case RETRO_ENVIRONMENT_GET_CAN_DUPE:
    bval =  reinterpret_cast<bool *>(data);
    *bval = true;
    return true;

  case RETRO_ENVIRONMENT_GET_LIBRETRO_PATH:
  {
    const char **dir = reinterpret_cast<const char **>(data);
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
    auto **ppDir =reinterpret_cast<char **>(data);
    *ppDir = sys_path;
    return true;
  }
  case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
  {
    std::string str = retro->saves_path;
    static auto *sys_path = strdup(str.c_str());
    auto **ppDir = reinterpret_cast<const char **>(data);
    *ppDir = sys_path;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK:
  {
    auto *callback =  reinterpret_cast<const struct retro_keyboard_callback *>(data);
    core_kb_callback(callback->callback);
    return true;
  }

  case RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS: // 31
  {
    retro->init_inputvars(reinterpret_cast<struct retro_input_descriptor *>(data));
    return true;
  }

  case RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION:
  {
    auto *var = reinterpret_cast<unsigned *>(data);
    *var = 2;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS:
  {
    return retro->init_configvars_coreoptions(data, 1);
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL:
  {
    auto *lang = reinterpret_cast<struct retro_core_options_intl *>(data);
    return retro->init_configvars_coreoptions(lang->us, 1);
  }

  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2_INTL:
  {
    auto *lang = reinterpret_cast<struct retro_core_options_v2_intl *>(data);
    return retro->init_configvars_coreoptions(lang->us, 2);
  }
  case RETRO_ENVIRONMENT_SET_CORE_OPTIONS_V2:
  {
    return retro->init_configvars_coreoptions(data, 2);
  }

  case RETRO_ENVIRONMENT_SET_VARIABLES:
  {
    return retro->init_configvars(reinterpret_cast<struct retro_variable *>(data));
  }

  case RETRO_ENVIRONMENT_GET_VARIABLE:
  {
    auto *var = reinterpret_cast<struct retro_variable *>(data);
    var->value = retro->load_corevars(var);
    return true;
  }
  case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE:
  {
    bool* vars = reinterpret_cast<bool *>(data);
    *vars = retro->variables_changed;
    retro->variables_changed = false;
    return true;
  }
  case RETRO_ENVIRONMENT_GET_MESSAGE_INTERFACE_VERSION:
  {
  unsigned * dat = reinterpret_cast<unsigned *>(data);
  *dat = 1;
    return true;
  }

  case RETRO_ENVIRONMENT_SET_MESSAGE:
  {
    auto *msg = reinterpret_cast<const struct retro_message *>(data);
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