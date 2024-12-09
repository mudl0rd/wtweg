#ifndef io_h_
#define io_h_

#include <libretro.h>
#include <SDL2/SDL.h>
#include <string>
#include <array>
#include <iostream>
#include <filesystem>
#include "clibretro.h"

struct vp
{
    unsigned x;
    unsigned y;
    unsigned width;
    unsigned height;
};

static std::array <std::string,24>retro_descripts=
{
     "\"RetroPad\" B",
    "\"RetroPad\" Y",
    "\"RetroPad\" Select",
    "\"RetroPad\" Start",
    "\"RetroPad\" Up",
    "\"RetroPad\" Down",
    "\"RetroPad\" Left",
    "\"RetroPad\" Right",
    "\"RetroPad\" A",
    "\"RetroPad\" X",
    "\"RetroPad\" L",
    "\"RetroPad\" R",
    "\"RetroPad\" L2",
    "\"RetroPad\" R2",
    "\"RetroPad\" L3",
    "\"RetroPad\" R3",
    "\"RetroPad\" Left Stick X Axis",
    "\"RetroPad\" Left Stick Y Axis",
    "\"RetroPad\" Right Stick X Axis",
    "\"RetroPad\" Right Stick Y Axis",
    "\"RetroPad\" Analog L2",
    "\"RetroPad\" Analog R2",
    "\"RetroPad\" Analog L3",
    "\"RetroPad\" Analog R3"
};

void video_render(int width,int height);
void video_restoresz();
vp resize_cb();
bool video_init(struct retro_game_geometry *geom, SDL_Window *window);
void video_refresh(const void *data, unsigned width,unsigned height, size_t pitch);
bool video_sethw(struct retro_hw_render_callback *hw);
void video_deinit();
void video_changegeom(struct retro_game_geometry *geom);
void audio_changeratefps(double refreshra, float input_srate, double fps);
uintptr_t video_get_fb();
bool video_set_pixelformat(retro_pixel_format fmt);



#endif