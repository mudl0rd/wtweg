#ifndef outvid_h_
#define outvid_h_

#include <SDL2/SDL.h>
#include "libretro.h"
#include "inout.h"
#ifdef USE_RPI
#include "glad_es.h"
#else
#include "glad.h"
#endif
#include <vector>
#include <numeric>

struct vp
{
    unsigned x;
    unsigned y;
    unsigned width;
    unsigned height;
};

class out_video
{
private:
    GLuint tex_id;
    GLuint rbo_id;
    
    bool software_rast;
    bool integer_scale;
    GLuint pitch;
    GLint tex_w, tex_h;
    GLuint base_w, base_h;
    GLuint current_w, current_h;
    unsigned rend_width, rend_height;
    retro_pixel_format pixfmt;
    uint8_t *temp_pixbuf;
    float aspect;
    struct
    {
        GLuint pixfmt;
        GLuint pixtype;
        GLuint bpp;
    } pixformat;

    void init_fb(int width, int height);

public:
    GLuint fbo_id;
    struct retro_hw_render_callback hw;
    vp resize_cb();
    bool init(struct retro_game_geometry *geom);
    void destroy();
    void changegeom(struct retro_game_geometry *geom);
    bool setpixfmt(retro_pixel_format fmt);
    bool sethwcb(struct retro_hw_render_callback *hw);
    void render(int width, int height);
    void refresh(const void *data, unsigned width, unsigned height, size_t pitch);
};

#endif