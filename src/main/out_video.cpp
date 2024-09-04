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
	retro_pixel_format pixfmt;
	uint8_t *temp_pixbuf;
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
	if (g_video.temp_pixbuf)
	{
		free(g_video.temp_pixbuf);
		g_video.temp_pixbuf = NULL;
	}
	g_video.temp_pixbuf = (uint8_t *)malloc(width * height * sizeof(uint16_t));

#ifndef USE_RPI
	glCreateTextures(GL_TEXTURE_2D, 1, &g_video.tex_id);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(g_video.tex_id, 1, GL_RGBA8, width, height);
#else
	glGenTextures(1, &g_video.tex_id);
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0,
				 g_video.pixformat.pixtype, g_video.pixformat.pixfmt, NULL);

#endif
	void init_framebuffer(int width, int height);
	init_framebuffer(width, height);
}

void video_changegeom(struct retro_game_geometry *geom)
{
	if (g_video.tex_w > geom->max_width || g_video.tex_h > geom->max_height)
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
#ifndef USE_RPI
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
#else
	switch (fmt)
	{
	case RETRO_PIXEL_FORMAT_XRGB8888:
		g_video.pixformat.pixfmt = GL_UNSIGNED_INT;
		g_video.pixformat.pixtype = GL_RGBA;
		g_video.pixformat.bpp = sizeof(uint32_t);
		break;
	case RETRO_PIXEL_FORMAT_0RGB1555:
	case RETRO_PIXEL_FORMAT_RGB565:
		g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
		g_video.pixformat.pixtype = GL_RGB;
		g_video.pixformat.bpp = sizeof(uint16_t);
		break;
	default:
		break;
	}
#endif
	g_video.pixfmt = fmt;
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
#ifndef USE_RPI
	if (hw->context_type == RETRO_HW_CONTEXT_OPENGL || hw->context_type == RETRO_HW_CONTEXT_OPENGL_CORE)
	{
#else
	if (hw->context_type == RETRO_HW_CONTEXT_OPENGLES3 || hw->context_type == RETRO_HW_CONTEXT_OPENGLES2)
	{
#endif
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

#ifdef USE_RPI
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
#else
	glCreateFramebuffers(1, &g_video.fbo_id);
	glNamedFramebufferTexture(g_video.fbo_id, GL_COLOR_ATTACHMENT0, g_video.tex_id, 0);
	if (g_video.hw.depth)
	{
		glCreateRenderbuffers(1, &g_video.rbo_id);
		glNamedRenderbufferStorage(g_video.rbo_id, g_video.hw.stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24,
								   width, height);
		glNamedFramebufferRenderbuffer(g_video.fbo_id, g_video.hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, g_video.rbo_id);
	}
	glCheckFramebufferStatus(GL_FRAMEBUFFER);
#endif
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
#ifndef USE_RPI
			g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
			g_video.pixformat.pixtype = GL_BGRA;
#else
			g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
			g_video.pixformat.pixtype = GL_RGB;
#endif
			g_video.pixformat.bpp = sizeof(uint16_t);
		}
	}
	else
	{
		g_video.pixformat.pixfmt = GL_UNSIGNED_BYTE;
		g_video.pixformat.pixtype = GL_RGBA;
		g_video.pixformat.bpp = sizeof(uint16_t);
	}

	if (g_video.temp_pixbuf)
	{
		free(g_video.temp_pixbuf);
		g_video.temp_pixbuf = NULL;
	}
	g_video.temp_pixbuf = (uint8_t *)malloc(geom->max_width * geom->max_height * sizeof(uint16_t));

#ifndef USE_RPI
	glCreateTextures(GL_TEXTURE_2D, 1, &g_video.tex_id);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(g_video.tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(g_video.tex_id, 1, GL_RGBA8, geom->max_width, geom->max_height);
#else
	glGenTextures(1, &g_video.tex_id);
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
				 g_video.pixformat.pixtype, g_video.pixformat.pixfmt, NULL);
#endif

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
#ifndef USE_RPI
	GLint dst_x0 = vpx.x;
	GLint dst_x1 = dst_x0 + vpx.width;
	GLint dst_y0 = (g_video.software_rast) ? (vpx.y + vpx.height) : vpx.y;
	GLint dst_y1 = (g_video.software_rast) ? vpx.y : (vpx.y + vpx.height);
	glBlitNamedFramebuffer(g_video.fbo_id, 0, 0, 0, g_video.current_w, g_video.current_h,
						   dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
	glBindFramebuffer(GL_READ_FRAMEBUFFER, g_video.fbo_id);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	GLint dst_x0 = vpx.x;
	GLint dst_x1 = dst_x0 + vpx.width;
	GLint dst_y0 = (g_video.software_rast) ? (vpx.y + vpx.height) : vpx.y;
	GLint dst_y1 = (g_video.software_rast) ? vpx.y : (vpx.y + vpx.height);
	glBlitFramebuffer(0, 0, g_video.current_w, g_video.current_h,
					  dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#endif
}

void conv_0rgb1555_rgb565(void *output_, const void *input_,
						  int width, int height,
						  int out_stride, int in_stride)
{
	int h;
	const uint16_t *input = (const uint16_t *)input_;
	uint16_t *output = (uint16_t *)output_;

#if defined(__SSE2__)
	int max_width = width - 7;

	const __m128i hi_mask = _mm_set1_epi16(
		(int16_t)((0x1f << 11) | (0x1f << 6)));
	const __m128i lo_mask = _mm_set1_epi16(0x1f);
	const __m128i glow_mask = _mm_set1_epi16(1 << 5);
#endif

	for (h = 0; h < height;
		 h++, output += out_stride >> 1, input += in_stride >> 1)
	{
		int w = 0;
#if defined(__SSE2__)
		for (; w < max_width; w += 8)
		{
			const __m128i in = _mm_loadu_si128((const __m128i *)(input + w));
			__m128i rg = _mm_and_si128(_mm_slli_epi16(in, 1), hi_mask);
			__m128i b = _mm_and_si128(in, lo_mask);
			__m128i glow = _mm_and_si128(_mm_srli_epi16(in, 4), glow_mask);
			_mm_storeu_si128((__m128i *)(output + w),
							 _mm_or_si128(rg, _mm_or_si128(b, glow)));
		}
#endif

		for (; w < width; w++)
		{
			uint16_t col = input[w];
			uint16_t rg = (col << 1) & ((0x1f << 11) | (0x1f << 6));
			uint16_t b = col & 0x1f;
			uint16_t glow = (col >> 4) & (1 << 5);
			output[w] = rg | b | glow;
		}
	}
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
#ifndef USE_RPI
		glBindTextureUnit(0, g_video.tex_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
		glTextureSubImage2D(g_video.tex_id, 0, 0, 0, width, height, g_video.pixformat.pixtype,
							g_video.pixformat.pixfmt, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
#else
		if (g_video.pixfmt == RETRO_PIXEL_FORMAT_0RGB1555)
		{
			memset(g_video.temp_pixbuf, 0, width * height * sizeof(uint16_t));
			conv_0rgb1555_rgb565(g_video.temp_pixbuf, data, width, height,
								 width * get_alignment(pitch), pitch);
			glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
			glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
			glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixformat.pixtype,
							g_video.pixformat.pixfmt, g_video.temp_pixbuf);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			return;
		}
		else if (g_video.pixfmt == RETRO_PIXEL_FORMAT_XRGB8888)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
   			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
		}

		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixformat.pixtype,
						g_video.pixformat.pixfmt, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

#endif
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
	if (g_video.temp_pixbuf)
	{
		free(g_video.temp_pixbuf);
		g_video.temp_pixbuf = NULL;
	}
}
