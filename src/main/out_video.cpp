#include <SDL2/SDL.h>
#include "libretro.h"
#include "inout.h"
#include "glad.h"
#include <vector>
#include <numeric>
struct
{
	GLuint tex_id;
	GLuint rbo_id;
	GLuint fbo_id;
	bool software_rast;
	bool integer_scale;
	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint base_w, base_h;
	GLuint current_w, current_h;
	unsigned rend_width, rend_height;
	float aspect;
	struct
	{
		GLuint pixfmt;
		GLuint pixtype;
		GLuint bpp;
	} pixformat;

	struct retro_hw_render_callback hw;
	SDL_Window *sdl_context;
} g_video = {0};

void video_setsize(unsigned width, unsigned height)
{
	g_video.rend_width = width;
	g_video.rend_height = height;
}

void reinit_fbo(int width, int height)
{
	if (g_video.tex_id)
		glDeleteTextures(1, &g_video.tex_id);

	glCreateTextures(GL_TEXTURE_2D, 1, &g_video.tex_id);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(g_video.tex_id, 1, GL_RGBA8, width, height);
	void init_framebuffer(int width, int height);
	init_framebuffer(width, height);
}

void video_changegeom(struct retro_game_geometry *geom)
{
	if (g_video.tex_w>geom->max_width ||g_video.tex_h>geom->max_height)
	{
		reinit_fbo(geom->max_width, geom->max_height);
		g_video.tex_w = geom->max_width;
		g_video.tex_h = geom->max_height;
	}
	g_video.base_w = geom->base_width;
	g_video.base_h = geom->base_height;
	if (geom->aspect_ratio <= 0.0)
		g_video.aspect = (float)geom->base_width / (float)geom->base_height;
	else
		g_video.aspect = geom->aspect_ratio;
}

bool video_set_pixelformat(retro_pixel_format fmt)
{
	switch (fmt)
	{
	case RETRO_PIXEL_FORMAT_0RGB1555:
		g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		g_video.pixformat.pixtype = GL_BGRA;
		g_video.pixformat.bpp = sizeof(uint16_t);
		break;
	case RETRO_PIXEL_FORMAT_XRGB8888:
		g_video.pixformat.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
		g_video.pixformat.pixtype = GL_BGRA;
		g_video.pixformat.bpp = sizeof(uint32_t);
		break;
	case RETRO_PIXEL_FORMAT_RGB565:
		g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
		g_video.pixformat.pixtype = GL_RGB;
		g_video.pixformat.bpp = sizeof(uint16_t);
		break;
	default:
		break;
	}
	return true;
}

uintptr_t video_get_fb()
{
	return g_video.fbo_id;
}

void video_integerscale(bool yes)
{
	g_video.integer_scale = yes;
}

void video_bindfb()
{
	if (g_video.software_rast)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, g_video.fbo_id);
		glViewport(0, 0, g_video.current_w, g_video.current_h);
		glScissor(0, 0, g_video.current_w, g_video.current_h);
	}
}

bool video_sethw(struct retro_hw_render_callback *hw)
{
	if (hw->context_type == RETRO_HW_CONTEXT_OPENGL || hw->context_type == RETRO_HW_CONTEXT_OPENGL_CORE)
	{
		hw->get_current_framebuffer = video_get_fb;
		hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
		g_video.hw = *hw;
		return true;
	}
	return false;
}

void init_framebuffer(int width, int height)
{
	if (g_video.fbo_id)
		glDeleteFramebuffers(1, &g_video.fbo_id);
	if (g_video.rbo_id)
		glDeleteRenderbuffers(1, &g_video.rbo_id);

	glCreateFramebuffers(1, &g_video.fbo_id);
	glNamedFramebufferTexture(g_video.fbo_id, GL_COLOR_ATTACHMENT0, g_video.tex_id, 0);

	if (g_video.hw.depth)
	{
		glCreateRenderbuffers(1, &g_video.rbo_id);
		glNamedRenderbufferStorage(g_video.rbo_id, g_video.hw.stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24,
								   width, height);
		glNamedFramebufferRenderbuffer(g_video.fbo_id,  g_video.hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT
													 : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_video.rbo_id);
	}

	glCheckFramebufferStatus(GL_FRAMEBUFFER);
}

vp resize_cb()
{
	vp vp_ = {0};
	unsigned x = 0;
	unsigned y = 0;
	unsigned height = g_video.current_h;
	unsigned width = height * g_video.aspect;
	if (width > g_video.current_w)
	{
		height = g_video.current_w / g_video.aspect;
		width = g_video.current_w;
	}
	if (!height || !width)
		return vp_;
	unsigned max_scale = (unsigned)std::min(g_video.rend_width / width,
											g_video.rend_height / height);
	if (!max_scale)
	{
		height = g_video.base_h;
		width = height * g_video.aspect;
		max_scale = (unsigned)std::min(g_video.rend_width / width,
									   g_video.rend_height / height);
	}
	width *= max_scale;
	height *= max_scale;
	x = SDL_floor(g_video.rend_width - width) / 2;
	y = SDL_floor(g_video.rend_height - height) / 2;
	vp_ = {x, y, width, height};
	return vp_;
}

bool video_init(struct retro_game_geometry *geom, SDL_Window *context)
{
	g_video.software_rast = !g_video.hw.context_reset;
	g_video.sdl_context = context;

	if (g_video.tex_id)
		glDeleteTextures(1, &g_video.tex_id);

	g_video.tex_id = 0;
	g_video.integer_scale = true;

	if (g_video.software_rast)
	{
		if (!g_video.pixformat.pixfmt)
		{
			g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			g_video.pixformat.pixtype = GL_BGRA;
			g_video.pixformat.bpp = sizeof(uint16_t);
		}
	}
	else
	{
		g_video.pixformat.pixfmt = GL_UNSIGNED_BYTE;
		g_video.pixformat.pixtype = GL_RGBA;
		g_video.pixformat.bpp = sizeof(uint16_t);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &g_video.tex_id);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(g_video.tex_id, 1, GL_RGBA8, geom->max_width, geom->max_height);

	init_framebuffer(geom->max_width, geom->max_height);

	g_video.tex_w = geom->max_width;
	g_video.tex_h = geom->max_height;
	g_video.base_w = geom->base_width;
	g_video.base_h = geom->base_height;
	if (geom->aspect_ratio <= 0.0)
		g_video.aspect = (float)geom->base_width / (float)geom->base_height;
	else
		g_video.aspect = geom->aspect_ratio;

	if (g_video.hw.context_reset)
		g_video.hw.context_reset();

	video_restoresz();

	return true;
}

void video_restoresz()
{
	SDL_Rect display_bounds;
	SDL_GetDisplayUsableBounds(0, &display_bounds);
	g_video.rend_width = display_bounds.w * 7 / 8, g_video.rend_height = display_bounds.h * 7 / 8;
	SDL_SetWindowSize((SDL_Window *)g_video.sdl_context, g_video.rend_width, g_video.rend_height);
	SDL_SetWindowPosition((SDL_Window *)g_video.sdl_context, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

static inline unsigned get_alignment(unsigned pitch)
{
	if (pitch & 1)
		return 1;
	if (pitch & 2)
		return 2;
	if (pitch & 4)
		return 4;
	return 8;
}

void video_render()
{
	vp vpx = resize_cb();
	GLint dst_x0 = vpx.x;
	GLint dst_x1 = dst_x0 + vpx.width;
	GLint dst_y0 = (g_video.software_rast) ? (vpx.y + vpx.height) : vpx.y;
	GLint dst_y1 = (g_video.software_rast) ? vpx.y : (vpx.y + vpx.height);
	glBlitNamedFramebuffer(g_video.fbo_id, 0, 0, 0, g_video.current_w, g_video.current_h,
						   dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}

void video_refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
	if (data == NULL)
		return;
	if (g_video.current_w != width || g_video.current_h != height)
	{
		g_video.current_h = height;
		g_video.current_w = width;
	}

	if (data != RETRO_HW_FRAME_BUFFER_VALID)
	{
		glBindTextureUnit(0, g_video.tex_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
		glTextureSubImage2D(g_video.tex_id, 0, 0, 0, width, height, g_video.pixformat.pixtype,
							g_video.pixformat.pixfmt, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
}

void video_deinit()
{
	if (g_video.hw.context_destroy)
	{
		g_video.hw.context_destroy();
	}
	memset(&g_video.hw, 0, sizeof(struct retro_hw_render_callback));

	if (g_video.tex_id)
	{
		glDeleteTextures(1, &g_video.tex_id);
		g_video.tex_id = 0;
	}
	if (g_video.fbo_id)
	{
		glDeleteFramebuffers(1, &g_video.fbo_id);
		g_video.fbo_id = 0;
	}

	if (g_video.rbo_id)
	{
		glDeleteRenderbuffers(1, &g_video.rbo_id);
		g_video.rbo_id = 0;
	}
}