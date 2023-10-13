#ifndef io_h_
#define io_h_

#include <libretro.h>
#include <SDL2/SDL.h>
#include <string>
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

static const char *retro_descripts[] = {
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
    "\"RetroPad\" Right Stick Y Axis"};

void audio_mix(const int16_t *samples, size_t size);
bool audio_init(float refreshra, float input_srate, float fps);
void audio_flush();
void audio_destroy();
void video_render();
void video_bindfb();
void video_restoresz();
vp resize_cb();
bool video_init(struct retro_game_geometry *geom, SDL_Window *window);
void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch);
void video_setsize(unsigned width, unsigned height);
bool video_sethw(struct retro_hw_render_callback *hw);
void video_deinit();
void video_integerscale(bool yes);
void video_changegeom(struct retro_game_geometry *geom);
void audio_changeratefps(float refreshra, float input_srate, float fps);
uintptr_t video_get_fb();
static retro_keyboard_event_t inp_keys = NULL;
bool video_set_pixelformat(retro_pixel_format fmt);

void init_inpt();
void init_inp(int num);
void close_inp(int num);
void reset_inpt();
void close_inpt();

void checkbuttons_forui(int selected_inp, bool *isselected_inp, int port);
int16_t input_state(unsigned port, unsigned device, unsigned index,
                    unsigned id);
void poll_lr();

bool load_inpcfg(retro_input_descriptor *var);
bool loadinpconf(uint32_t checksum);
bool save_inpcfg(uint32_t checksum);

#endif