#ifndef CEMULATOR_H
#define CEMULATOR_H

#include <list>
#include <mutex>
#include <vector>
#include "io.h"

struct loadedcore_configcat
{
	std::string key;
	std::string desc;
	std::string info;
	bool visible;
};

struct loadedcore_configvars
{
	std::string name;
	std::string var;
	std::string description;
	std::string desc_cat;
	std::string tooltip;
	std::string tooltip_cat;
	std::string usevars;
	std::string category_name;
	std::vector<std::string> config_vals;
	std::vector<std::string> config_vals_desc;
	std::string default_val;
	unsigned sel_idx;
	bool config_visible;
};

struct core_info
{
	std::string core_extensions;
	std::string core_ext_filter;
	std::string core_name;
	std::string core_path;
	float aspect_ratio;
	float samplerate;
	float fps;
	bool no_roms;
};

struct retro_core
{
	void *handle;
	bool initialized;
	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
	bool (*retro_load_game)(const struct retro_game_info *game);
	void *(*retro_get_memory_data)(unsigned id);
	size_t (*retro_get_memory_size)(unsigned id);
	void (*retro_unload_game)(void);
};

enum joytype_
{
	joystick_,
	button,
	keyboard,
	hat
};

struct coreinput_desc
{
	std::string desc;
	unsigned id;
};

struct coreinput_bind
{
	int SDL_port;
	int port;
	std::string description;
	std::string joykey_desc;
	int16_t val;
	uint8_t retro_id;
	uint8_t isanalog;
	// config vars
	union conf
	{
		uint16_t val;
		struct config
		{
			uint8_t sdl_id;
			uint8_t joytype;
			int16_t axistrigger;
		} bits;
	} config;
};

enum libretro_padbinds
{
	joypad_b,
	joypad_y,
	joypad_select,
	joypad_start,
	joypad_up,
	joypad_down,
	joypad_left,
	joypad_right,
	joypad_a,
	joypad_x,
	joypad_l,
	joypad_r,
	joypad_l2,
	joypad_r2,
	joypad_l3,
	joypad_r3
};

enum libretro_stickbinds
{
	joypad_analogx_l = 16,
	joypad_analogy_l,
	joypad_analogx_r,
	joypad_analogy_r
};

enum libretro_stickdirs
{
	joypad_analogupl = 0x7C4,
	joypad_analogleftl,
	joypad_analogrightr,
	joypad_analogdownl,
	joypad_analogupr,
	joypad_analogleftr,
	joypad_analogrightl,
	joypad_analogdownr,
	joypad_ltrigger,
	joypad_rtrigger
};

struct retro_disk
{
	int index;
	std::string path;
};

std::vector<core_info> get_cores();

class CLibretro
{
private:
	retro_core retro;
	void load_envsymb(void *handle, bool first);
	SDL_Window *sdl_window;

public:
	int controller_type[2];
	bool lr_isrunning;
	CLibretro(SDL_Window *window, char *exepath);
	~CLibretro();
	CLibretro(CLibretro const &) = delete;
	CLibretro &operator=(CLibretro const &) = delete;
	static std::shared_ptr<CLibretro> get_classinstance(SDL_Window *window = NULL, char *exepath = NULL)
	{
		static std::shared_ptr<CLibretro> s{new CLibretro(window, exepath)};
		return s;
	}

	void poll();

	void core_changinpt(int dev, int port);
	bool core_isrunning();
	bool core_load(char *ROM, bool game_specific_settings, char *corepath, bool contentless);
	void core_unload();
	bool core_saveram(const char *filename, bool save);
	bool core_savestateslot(bool save);
	bool core_savestate(const char *filename, bool save);
	void core_run();
	void set_inputdevice(int device);
	void get_cores();

	bool init_configvars(retro_variable *var);
	bool init_configvars_coreoptions(void *var, int version);
	bool init_inputvars(retro_input_descriptor *var);
	bool load_coresettings();
	void save_coresettings();

	const char *load_corevars(retro_variable *var);

	std::vector<coreinput_bind> core_inputbinds[2];
	std::vector<coreinput_desc> core_inputdesc[2];
	std::vector<loadedcore_configvars> core_variables;
	std::vector<loadedcore_configcat> core_categories;
	std::vector<retro_disk> disk_intf;

	bool v2_vars;
	bool variables_changed;
	std::vector<core_info> cores;
	std::string romsavesstatespath;
	std::string core_config;
	std::string core_path;
	std::string saves_path;
	std::string system_path;
	std::string rom_path;
	std::string coreexts;
	std::string exe_path;
	int save_slot;
	float core_fps;
	double coretime;
	float refreshrate;
	struct retro_perf_counter *perf_counter_last;
};

bool loadfile(CLibretro *instance, const char *file, const char *core_file, bool pergame);
void sdlggerat_menu(CLibretro *instance, std::string *window_str);
void add_log(enum retro_log_level level, const char *fmt);

#endif