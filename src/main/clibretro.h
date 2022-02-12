#ifndef CEMULATOR_H
#define CEMULATOR_H

#include <list>
#include <mutex>
#include <vector>
#include "io.h"

    struct loadedcore_configvars
	{
		std::string name;
		std::string var;
		std::string description;
		std::string usevars;
		std::vector<std::string>config_vals;
		int sel_idx;
		bool config_visible;
	};

    struct core_info{
        std::string core_extensions;
		std::string core_ext_filter;
        std::string core_name;
       std::string core_path;
       float aspect_ratio;
        float samplerate;
        float fps;
    };

struct retro_core{
		void* handle;
		bool initialized;
		void(*retro_init)(void);
		void(*retro_deinit)(void);
		unsigned(*retro_api_version)(void);
		void(*retro_get_system_info)(struct retro_system_info* info);
		void(*retro_get_system_av_info)(struct retro_system_av_info* info);
		void(*retro_set_controller_port_device)(unsigned port, unsigned device);
		void(*retro_reset)(void);
		void(*retro_run)(void);
		size_t(*retro_serialize_size)(void);
		bool(*retro_serialize)(void* data, size_t size);
		bool(*retro_unserialize)(const void* data, size_t size);
		bool(*retro_load_game)(const struct retro_game_info* game);
		void* (*retro_get_memory_data)(unsigned id);
		size_t(*retro_get_memory_size)(unsigned id);
		void(*retro_unload_game)(void);
	};

	enum joytype{
		joystick_,
		button,
		keyboard,
		hat
	};

	struct coreinput_bind {
         std::string description;
		 std::string joykey_desc;
		  unsigned sdl_id;
		  SDL_JoystickGUID joystic_guid;
	      std::string joystick_name;
		  bool isanalog;
		  int axises;
		  int val;
		  joytype joytype;
		  
      };

enum libretro_padbinds{
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
	joypad_analogx_r,
	joypad_analogy_l,
	joypad_analogy_r
};

std::vector<core_info> get_cores();

class CLibretro
{
    private:	
   
    retro_core retro;
	struct retro_game_info info;
	void load_envsymb(void* handle);
	SDL_Window * sdl_window;
    public:
    bool lr_isrunning;
    CLibretro(SDL_Window *window);
	~CLibretro();
    static CLibretro* get_classinstance(SDL_Window* window = NULL);


    void poll();
    
    bool core_isrunning();
    bool core_load(char* ROM, bool game_specific_settings);
    void core_unload();
    void core_run();
    void set_inputdevice(int device);
	void get_cores();
	

	bool init_configvars(retro_variable *var);
	bool init_inputvars(retro_input_descriptor* var);
	bool load_coresettings();
	void save_coresettings();
    
	const char* load_corevars(retro_variable *var);
	

	
    std::vector<coreinput_bind>core_inputbinds;
    std::vector<loadedcore_configvars> core_variables;
	bool variables_changed;
	std::vector<core_info> cores;
	std::string romsavesstatespath;
	std::string corespath;
	std::string core_path;
};


void sdlggerat_menu(CLibretro *instance, std::string *window_str, int * selected_in,bool *isselected_inp);



#endif