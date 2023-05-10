#include <SDL2/SDL.h>
#include "libretro.h"
#include "io.h"
#include "glad.h"
#include <vector>

struct
{
	GLuint tex_id;
	GLuint rbo_id;
	GLuint fbo_id;
	bool software_rast;
	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint base_w, base_h;
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

void video_changegeom(struct retro_game_geometry *geom)
{
	g_video.tex_w = geom->max_width;
	g_video.tex_h = geom->max_height;
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

void video_bindfb()
{
		int w = (!g_video.software_rast)?g_video.base_w:g_video.rend_width;
		int h = (!g_video.software_rast)?g_video.base_w:g_video.rend_height;
		glBindFramebuffer(GL_FRAMEBUFFER, g_video.fbo_id);
		glViewport(0, 0, w, h);
		glScissor(0, 0, w, h);
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

	glGenFramebuffers(1, &g_video.fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, g_video.fbo_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   g_video.tex_id, 0);

	if (g_video.hw.depth)
	{
		glGenRenderbuffers(1, &g_video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, g_video.rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER,
							  g_video.hw.stencil ? GL_DEPTH24_STENCIL8
												 : GL_DEPTH_COMPONENT24,
							  width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
								  g_video.hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT
													 : GL_DEPTH_ATTACHMENT,
								  GL_RENDERBUFFER, g_video.rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

vp resize_cb()
{
	vp vp_ = {0};
	float aspect = g_video.aspect;
	unsigned height = g_video.rend_height;
	unsigned width = height * aspect;
	if (width > g_video.rend_width)
	{
		height = g_video.rend_width / aspect;
		width = g_video.rend_width;
	}
	unsigned x = SDL_floor(g_video.rend_width - width) / 2;
	unsigned y = SDL_floor(g_video.rend_height - height) / 2;
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

	glGenTextures(1, &g_video.tex_id);
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
				 g_video.pixformat.pixtype, g_video.pixformat.pixfmt, NULL);

	init_framebuffer(geom->max_width, geom->max_height);
	video_changegeom(geom);

	if (g_video.hw.context_reset)
		g_video.hw.context_reset();

	if (!g_video.rend_width)
		SDL_GetWindowSize((SDL_Window *)g_video.sdl_context, (int *)&g_video.rend_width, (int *)&g_video.rend_height);
	SDL_SetWindowPosition((SDL_Window *)g_video.sdl_context, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	vp vpx = resize_cb();

	SDL_SetWindowSize((SDL_Window *)g_video.sdl_context, vpx.width,
					  vpx.height);

	return true;
}

void video_restoresz()
{
	SDL_Rect display_bounds;
	SDL_GetDisplayUsableBounds(0, &display_bounds);
	g_video.rend_width = display_bounds.w * 7 / 8, g_video.rend_height = display_bounds.h * 7 / 8;
	vp vpx = resize_cb();
	SDL_SetWindowSize((SDL_Window *)g_video.sdl_context, vpx.width, vpx.height);
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
		
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_video.fbo_id);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	int dest_x0 = vpx.x;
	int dest_x1 = vpx.x + vpx.width;
	int dest_y0 = (g_video.software_rast) ? (vpx.y + vpx.height) : vpx.y;
	int dest_y1 = (g_video.software_rast) ? vpx.y : (vpx.y + vpx.height);
	glBlitFramebuffer(0, 0, g_video.base_w, g_video.base_h,
					  dest_x0, dest_y0, dest_x1, dest_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch)
{
	if (data == NULL)
		return;
	if (g_video.base_w != width || g_video.base_h != height)
	{
		g_video.base_h = height;
		g_video.base_w = width;
	}
	if (data != RETRO_HW_FRAME_BUFFER_VALID)
	{
		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixformat.pixtype,
						g_video.pixformat.pixfmt, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
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