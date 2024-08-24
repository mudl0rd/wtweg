#ifndef io_h_
#define io_h_

#include <libretro.h>
#include <SDL2/SDL.h>
#include <string>
#include <array>
#include <iostream>
#include <filesystem>
#include "clibretro.h"
#include "out_aud.h"

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

void video_render();
void video_bindfb();
void video_unbindfb();
void video_restoresz();
vp resize_cb();
bool video_init(struct retro_game_geometry *geom, SDL_Window *window);
void video_refresh(const void *data, unsigned width,unsigned height, size_t pitch);
void video_setsize(unsigned width, unsigned height);
bool video_sethw(struct retro_hw_render_callback *hw);
void video_deinit();
void video_integerscale(bool yes);
void video_changegeom(struct retro_game_geometry *geom);
void audio_changeratefps(float refreshra, float input_srate, float fps);
uintptr_t video_get_fb();

bool video_set_pixelformat(retro_pixel_format fmt);

void init_inpt();
void init_inp(int num);
void close_inp(int num);
void reset_inpt();
void close_inpt();

void core_kb_callback(retro_keyboard_event_t e);
void input_keys(uint16_t mod, uint16_t keycode, bool down);
void checkbuttons_forui(int selected_inp, bool *isselected_inp, int port);
int16_t input_state(unsigned port, unsigned device, unsigned index,
                    unsigned id);
void poll_lr();

bool load_inpcfg(retro_input_descriptor *var);
bool loadinpconf(uint32_t checksum,bool save_f);
bool loadcontconfig(bool save_f);

#endif