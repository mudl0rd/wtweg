#ifndef CEMULATOR_H
#define CEMULATOR_H

#include <list>
#include <mutex>
#include <vector>
#include "io.h"
#include <SDL2/SDL.h>

    struct core_configvars
	{
		std::string name;
		std::string var;
		std::string description;
		std::string usevars;
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

		 void (*retro_set_environment)(retro_environment_t);
  void (*retro_set_video_refresh)(retro_video_refresh_t);
  void (*retro_set_input_poll)(retro_input_poll_t);
  void (*retro_set_input_state)(retro_input_state_t);
  void (*retro_set_audio_sample)(retro_audio_sample_t);
  void (*retro_set_audio_sample_batch)(retro_audio_sample_batch_t);
	};


std::vector<core_info> get_cores();

class CLibretro
{
    private:	
   
    bool lr_isrunning;
    retro_core retro;
	struct retro_game_info info;
	void load_envsymb(void* handle);
	SDL_Window * sdl_window;
    public:
    CLibretro(SDL_Window *window);
	~CLibretro();
    static CLibretro* get_classinstance(SDL_Window* window = NULL);

    bool core_isrunning();
    bool core_load(char* ROM, bool game_specific_settings);
    void core_unload();
    void core_run();
    void set_inputdevice(int device);
	void get_cores();
	

	

    std::vector<core_configvars> core_variables;
	std::vector<core_info> cores;
};

#endif