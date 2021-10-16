#ifndef io_h_
#define io_h_

class libretro_render
{
public:
	virtual ~libretro_render() {};

	virtual bool init(const struct retro_game_geometry* geom, retro_pixel_format fmt, float & refreshrate) = 0;
	virtual void refresh(const void* data, unsigned width, unsigned height, unsigned pitch) = 0;
	virtual void set_hwcontext(retro_hw_render_callback* hw) = 0;
	virtual void resize(int rend_width, int rend_height) = 0;
	virtual void deinit() = 0;
	virtual void buf_clear() = 0;
	virtual uintptr_t get_fb() = 0;
};

#endif