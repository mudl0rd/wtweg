#ifndef io_h_
#define io_h_

#include <libretro.h>
#include <SDL2/SDL.h>
#include <string>
#include <iostream>
#include <filesystem>
#include "clibretro.h"

void audio_mix(const int16_t* samples, size_t size);
bool audio_init(float refreshra, float input_srate, float fps);
void audio_destroy();
void video_render();
bool video_init(const struct retro_game_geometry* geom, SDL_Window *window);
void video_refresh(const void* data, unsigned width, unsigned height, unsigned pitch);
void video_setsize(unsigned width,unsigned height);
bool video_sethw(struct retro_hw_render_callback *hw);
void video_deinit();
uintptr_t video_get_fb();
bool video_set_pixelformat(retro_pixel_format fmt);

void init_inp();
void close_inp();
bool poll_inp(int selected_inp,bool *isselected_inp);
int16_t input_state(unsigned port, unsigned device, unsigned index,
                                unsigned id);
void poll_lr();

bool load_inpcfg(retro_input_descriptor *var);
bool save_inpcfg();



#endif