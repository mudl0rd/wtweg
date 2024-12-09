#include <SDL2/SDL.h>
#include "clibretro.h"
#include "libretro.h"
#ifdef USE_RPI
#include "glad_es.h"
#else
#include "glad.h"
#endif
#include <vector>
#include <numeric>

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

static inline void conv_0rgb1555_rgb565(void *output_, const void *input_,
										int width, int height,
										int out_stride, int in_stride)
{
	int h;
	const uint16_t *input = (const uint16_t *)input_;
	uint16_t *output = (uint16_t *)output_;
	for (h = 0; h < height;
		 h++, output += out_stride >> 1, input += in_stride >> 1)
	{
		int w = 0;
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

static inline void conv_argb8888_rgba8888(void *output_, const void *input_,
										  int width, int height)
{
	const uint8_t *input = (const uint8_t *)input_;
	uint8_t *output = (uint8_t *)output_;

	int offset = 0;
	for (int x = 0; x < width * height; x++)
	{
		output[offset] = input[offset + 2];
		output[offset + 1] = input[offset + 1];
		output[offset + 2] = input[offset];
		output[offset + 3] = input[offset + 3];
		offset += 4;
	}
}

void out_video::init_fb(int width, int height)
{
	if (tex_id)
		glDeleteTextures(1, &tex_id);
	if (temp_pixbuf)
	{
		free(temp_pixbuf);
		temp_pixbuf = NULL;
	}
	temp_pixbuf = (uint8_t *)malloc(width * height * sizeof(uint32_t));

#ifndef USE_RPI
	glCreateTextures(GL_TEXTURE_2D, 1, &tex_id);
	glTextureParameteri(tex_id, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(tex_id, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureStorage2D(tex_id, 1, GL_RGBA8, width, height);
#else
	glGenTextures(1, &tex_id);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, (pixfmt == RETRO_PIXEL_FORMAT_XRGB8888) ? GL_RGBA : GL_RGB, width, height, 0,
				 pixformat.pixtype, pixformat.pixfmt, NULL);

#endif

	if (fbo_id)
		glDeleteFramebuffers(1, &fbo_id);
	if (rbo_id)
		glDeleteRenderbuffers(1, &rbo_id);

#ifdef USE_RPI
	glGenFramebuffers(1, &fbo_id);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   tex_id, 0);

	if (hw.depth)
	{
		glGenRenderbuffers(1, &rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, rbo_id);
		glRenderbufferStorage(GL_RENDERBUFFER,
							  hw.stencil ? GL_DEPTH24_STENCIL8
										 : GL_DEPTH_COMPONENT24,
							  width, height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER,
								  hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT
											 : GL_DEPTH_ATTACHMENT,
								  GL_RENDERBUFFER, rbo_id);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
	glCheckFramebufferStatus(GL_FRAMEBUFFER);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
#else
	glCreateFramebuffers(1, &fbo_id);
	glNamedFramebufferTexture(fbo_id, GL_COLOR_ATTACHMENT0, tex_id, 0);
	if (hw.depth)
	{
		glCreateRenderbuffers(1, &rbo_id);
		glNamedRenderbufferStorage(rbo_id, hw.stencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT24,
								   width, height);
		glNamedFramebufferRenderbuffer(fbo_id, hw.stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_id);
	}
	glCheckFramebufferStatus(GL_FRAMEBUFFER);
#endif
}

vp out_video::resize_cb()
{
	vp vp_ = {0};
	unsigned x = 0;
	unsigned y = 0;
	unsigned height = current_h;
	unsigned width = height * aspect;
	if (width > current_w)
	{
		height = current_w / aspect;
		width = current_w;
	}
	if (!height || !width)
		return vp_;

	unsigned max_scale = (unsigned)std::min(rend_width / width,
											rend_height / height);

	if (!max_scale)
	{
		if (base_h < height)
		{
			height = base_h;
			width = height * aspect;
			max_scale = (unsigned)std::min(rend_width / width,
										   rend_height / height);
		}
		else
		{
			height = rend_height;
			width = height * aspect;
			if (width > rend_width)
			{
				height = rend_width / aspect;
				width = rend_width;
			}
		}
	}
	width *= (!max_scale) ? 1 : max_scale;
	height *= (!max_scale) ? 1 : max_scale;

	x = SDL_floor(rend_width - width) / 2;
	y = SDL_floor(rend_height - height) / 2;
	vp_ = {x, y, width, height};
	return vp_;
}

bool out_video::init(struct retro_game_geometry *geom)
{
	software_rast = !hw.context_reset;

	if (tex_id)
		glDeleteTextures(1, &tex_id);

	tex_id = 0;
	integer_scale = true;

	if (software_rast)
	{
		if (!pixformat.pixfmt)
		{
			pixformat.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
			pixformat.pixtype = GL_RGB;
			pixformat.bpp = sizeof(uint16_t);
		}
	}
	else
	{
		pixformat.pixfmt = GL_UNSIGNED_BYTE;
		pixformat.pixtype = GL_RGBA;
		pixformat.bpp = sizeof(uint32_t);
	}

	init_fb(geom->max_width, geom->max_height);

	tex_w = geom->max_width;
	tex_h = geom->max_height;
	base_w = geom->base_width;
	base_h = geom->base_height;
	if (geom->aspect_ratio <= 0.0)
		aspect = (float)geom->base_width / (float)geom->base_height;
	else
		aspect = geom->aspect_ratio;

	if (hw.context_reset)
		hw.context_reset();

	return true;
}
void out_video::destroy()
{
	if (hw.context_destroy)
	{
		hw.context_destroy();
	}
	memset(&hw, 0, sizeof(struct retro_hw_render_callback));

	if (tex_id)
	{
		glDeleteTextures(1, &tex_id);
		tex_id = 0;
	}
	if (fbo_id)
	{
		glDeleteFramebuffers(1, &fbo_id);
		fbo_id = 0;
	}

	if (rbo_id)
	{
		glDeleteRenderbuffers(1, &rbo_id);
		rbo_id = 0;
	}
	if (temp_pixbuf)
	{
		free(temp_pixbuf);
		temp_pixbuf = NULL;
	}
}
void out_video::changegeom(struct retro_game_geometry *geom)
{
	if (tex_w > geom->max_width || tex_h > geom->max_height)
	{
		init_fb(geom->max_width, geom->max_height);
		tex_w = geom->max_width;
		tex_h = geom->max_height;
	}
	base_w = geom->base_width;
	base_h = geom->base_height;
	if (geom->aspect_ratio <= 0.0)
		aspect = (float)geom->base_width / (float)geom->base_height;
	else
		aspect = geom->aspect_ratio;
}

bool out_video::setpixfmt(retro_pixel_format fmt)
{
	switch (fmt)
	{
	case RETRO_PIXEL_FORMAT_XRGB8888:
		pixformat.pixfmt = GL_UNSIGNED_BYTE;
		pixformat.pixtype = GL_RGBA;
		pixformat.bpp = sizeof(uint32_t);
		break;
	case RETRO_PIXEL_FORMAT_0RGB1555:
	case RETRO_PIXEL_FORMAT_RGB565:
		pixformat.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
		pixformat.pixtype = GL_RGB;
		pixformat.bpp = sizeof(uint16_t);
		break;
	default:
		break;
	}
	pixfmt = fmt;
	return true;
}

uintptr_t out_video::getfb() { return fbo_id; }
bool out_video::sethwcb(struct retro_hw_render_callback *hw)
{
#ifndef USE_RPI
	if (hw->context_type == RETRO_HW_CONTEXT_OPENGL || hw->context_type == RETRO_HW_CONTEXT_OPENGL_CORE)
	{
#else
	if (hw->context_type == RETRO_HW_CONTEXT_OPENGLES3 || hw->context_type == RETRO_HW_CONTEXT_OPENGLES2)
	{
#endif
		static auto core_fb_callback = +[]() -> uintptr_t
		{
			CLibretro *inst = CLibretro::get_classinstance();
			return inst->video.getfb();
		};

		hw->get_current_framebuffer = core_fb_callback;
		hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
		this->hw = *hw;
		return true;
	}
	return false;
}

void out_video::render(int width, int height)
{
	rend_width = width;
	rend_height = height;
	vp vpx = resize_cb();
	GLint dst_x0 = vpx.x;
	GLint dst_x1 = dst_x0 + vpx.width;
	GLint dst_y0 = (software_rast) ? (vpx.y + vpx.height) : vpx.y;
	GLint dst_y1 = (software_rast) ? vpx.y : (vpx.y + vpx.height);
#ifndef USE_RPI
	glBlitNamedFramebuffer(fbo_id, 0, 0, 0, current_w, current_h,
						   dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);
#else
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_id);
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, current_w, current_h,
					  dst_x0, dst_y0, dst_x1, dst_y1, GL_COLOR_BUFFER_BIT, GL_NEAREST);

#endif
}
void out_video::refresh(const void *data, unsigned width, unsigned height, size_t pitch)
{
	if (data == NULL)
		return;
	if (current_w != width || current_h != height)
	{
		current_h = height;
		current_w = width;
	}

	if (data != RETRO_HW_FRAME_BUFFER_VALID)
	{
#ifndef USE_RPI
		glBindTextureUnit(0, tex_id);
#else
		glBindTexture(GL_TEXTURE_2D, tex_id);
#endif
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(pitch));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / pixformat.bpp);
		switch (pixfmt)
		{
		case RETRO_PIXEL_FORMAT_0RGB1555:
			memset(temp_pixbuf, 0, width * height * sizeof(uint16_t));
			conv_0rgb1555_rgb565(temp_pixbuf, data, width, height,
								 pitch, pitch);
#ifndef USE_RPI
			glTextureSubImage2D(tex_id, 0, 0, 0, width, height, pixformat.pixtype,
								pixformat.pixfmt, temp_pixbuf);
#else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, pixformat.pixtype,
							pixformat.pixfmt, temp_pixbuf);
#endif
			break;
		case RETRO_PIXEL_FORMAT_XRGB8888:
			memset(temp_pixbuf, 0, width * height * sizeof(uint32_t));
			conv_argb8888_rgba8888(temp_pixbuf, data, width, height);
#ifndef USE_RPI
			glTextureSubImage2D(tex_id, 0, 0, 0, width, height, pixformat.pixtype,
								pixformat.pixfmt, temp_pixbuf);
#else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, pixformat.pixtype,
							pixformat.pixfmt, temp_pixbuf);
#endif
			break;
		case RETRO_PIXEL_FORMAT_RGB565:
#ifndef USE_RPI
			glTextureSubImage2D(tex_id, 0, 0, 0, width, height, pixformat.pixtype,
								pixformat.pixfmt, data);
#else
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, pixformat.pixtype,
							pixformat.pixfmt, data);
#endif

			break;
		}
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
}