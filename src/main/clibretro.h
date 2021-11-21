#ifndef CEMULATOR_H
#define CEMULATOR_H

#include <list>
#include <mutex>
#include <vector>
#include "io.h"

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

std::vector<core_info> get_cores(const char* path);

class CLibretro
{
    private:	
	static	CLibretro* instance;
    libretro_render* render;
    bool lr_isrunning;


    struct {
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
	} retro_core;

    public:
    CLibretro(void *window);
	~CLibretro();
    static CLibretro* get_classinstance(void* window = NULL);

    bool core_isrunning();
    bool core_load(core_info* core, char* ROM, bool game_specific_settings);
    void core_unload();
    void core_run();
    void set_inputdevice(int device);

    std::vector<core_configvars> core_variables;
	std::vector<core_info> cores;
    

   


};

#endif