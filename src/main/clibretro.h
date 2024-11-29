#ifndef CEMULATOR_H
#define CEMULATOR_H

#include <list>
#include <mutex>
#include <vector>
#include "inout.h"
#include <thread>

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

struct coreinput_controlinfo
{
	std::string desc;
	unsigned id;
};

struct coreinput_bind
{
	int SDL_port;
	int port;
	int device;
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
			uint16_t sdl_id;
			uint8_t joytype;
			int16_t axistrigger;
		} bits;
	} config;
};

struct clibretro_startoptions
{
	std::string rom;
	std::string savestate;
	std::string core;
	bool game_specific_settings;
	bool framelimit;
};

enum libretro_binds
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
	joypad_r3,
	joypad_analogx_l,
	joypad_analogy_l,
	joypad_analogx_r,
	joypad_analogy_r,
	joypad_analog_l2,
	joypad_analog_r2,
	joypad_analog_l3,
	joypad_analog_r3,
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



struct controller_port
{
	std::vector<coreinput_bind> inputbinds;
	std::vector<coreinput_controlinfo> controlinfo;
	int controller_type;
};

struct subsystem_rominfo
{
	std::string memory_ext;
	unsigned memory_type;
	unsigned memory_id;
	std::string romtype;
	bool required;
	std::string romexts;
	bool need_fullpath;
	bool block_extract;
};

struct subsystems
{
	std::vector<subsystem_rominfo> rominfo;
	std::string subsystem_name;
	unsigned subsystem_id;
};

struct core_info
{
	std::string core_extensions;
	std::string core_ext_filter;
	std::string core_name;
	std::string core_path;
	std::vector<subsystems>subsystems;
	float aspect_ratio;
	float samplerate;
	float fps;
	bool no_roms;
};

std::vector<core_info> get_cores();

class CLibretro
{
private:
	retro_core retro;
	void load_envsymb(void *handle, bool first);
	SDL_Window *sdl_window;
	bool lr_isrunning;

public:
	CLibretro() = default;
	~CLibretro() = default;
	CLibretro(const CLibretro &) = delete;
	CLibretro &operator=(const CLibretro &) = delete;
	void init_lr(SDL_Window *window);
	static CLibretro *get_classinstance(SDL_Window *window = NULL)
	{
		static thread_local CLibretro instance;
		if (window)
			instance.init_lr(window);
		return &instance;
	}
	void poll();
	void reset();
	void core_changinpt(int dev, int port);
	bool core_isrunning();
	bool core_load(bool contentless, clibretro_startoptions *options);
	void core_unload();
	bool core_saveram(const char *filename, bool save);
	bool core_savestateslot(bool save);
	bool core_savestate(const char *filename, bool save);
	void core_run();
	void set_inputdevice(int device);
	void get_cores();
	void framelimit();
	void framecap(bool cap)
	{
		capfps = cap;
		void audio_framelimit(bool cap);
		audio_framelimit(cap);
	};

	bool init_configvars(retro_variable *var);
	bool init_configvars_coreoptions(void *var, int version);
	bool init_inputvars(retro_input_descriptor *var);
	bool load_coresettings(bool save_f);

	const char *load_corevars(retro_variable *var);

	std::vector<std::vector<coreinput_controlinfo>> core_inputttypes;
	std::vector<controller_port> core_inpbinds;
	std::vector<loadedcore_configvars> core_variables;
	std::vector<loadedcore_configcat> core_categories;
	std::vector<retro_disk> disk_intf;

	bool v2_vars;
	bool use_retropad;
	bool variables_changed;
	const char *rom_name;
	uint32_t config_crc;
	uint32_t input_confcrc;
	uint32_t inputcont_crc;
	std::vector<core_info> cores;
	std::string romsavesstatespath;
	std::string core_config;
	std::string core_path;
	std::filesystem::path rom_path;
	std::string coreexts;
	std::string exe_path;
	std::string load_savestate;
	int save_slot;
	struct retro_perf_counter *perf_counter_last;
	retro_frame_time_callback_t frametime_cb;
	retro_usec_t frametime_ref;
	uint64_t fps;
	uint64_t perfc;
	bool capfps;

	float deltatime;
	uint64_t frameno;
	float core_samplerate;
	float core_fps;
	float max_deltatime;
	float min_deltime;
	std::vector<float> frames;
	int subsystem_type;
};

bool loadfile(CLibretro *instance, clibretro_startoptions *options);
void sdlggerat_menu(CLibretro *instance, std::string *window_str);
void add_log(enum retro_log_level level, const char *fmt);
void rombrowse_setdir(std::string dir);

#endif