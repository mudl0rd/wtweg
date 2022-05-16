#include <SDL2/SDL.h>
#include "libretro.h"
#include "io.h"
#include "gl3w.h"
#include <vector>

static const char *g_vshader_src = 
R"(
#version 330
in vec2 i_pos;
in vec2 i_coord;
out vec2 o_coord;
uniform mat4 u_mvp;
void main() {
o_coord = i_coord;
gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;
})";

static const char *g_fshader_src = 
R"(
#version 330
in vec2 o_coord;
uniform sampler2D u_tex;
layout(location = 0) out vec4 FragColor;
void main() {
FragColor = texture(u_tex, o_coord);
})";

struct
{
	GLuint tex_id;
	GLuint rbo_id;
	GLuint fbo_id;

	int glmajor;
	int glminor;
	bool software_rast;
	GLuint pitch;
	GLint tex_w, tex_h;
	GLuint base_w, base_h;
	unsigned rend_width, rend_height;
	float aspect;
	int aspect_factor;

	struct
	{
		GLuint pixfmt;
		GLuint pixtype;
		GLuint bpp;

	} pixformat;

	struct retro_hw_render_callback hw;
	SDL_Window *sdl_context;

	struct
	{
		GLuint vao;
		GLuint vbo;
		GLuint program;
		GLuint fshader;
		GLuint vshader;
		GLint i_pos;
		GLint i_coord;
		GLint u_tex;
		GLint u_mvp;
	} g_shader = {0};
} g_video = {0};

void video_setsize(unsigned width, unsigned height)
{
	g_video.rend_width = width;
	g_video.rend_height = height;
}

bool video_set_pixelformat(retro_pixel_format fmt)
{
	memset(&g_video.hw, 0, sizeof(struct retro_hw_render_callback));
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

void ortho2d(float m[4][4], float left, float right, float bottom, float top)
{
	m[3][3] = 1;
	m[0][0] = 2.0f / (right - left);
	m[1][1] = 2.0f / (top - bottom);
	m[2][2] = -1.0f;
	m[3][0] = -(right + left) / (right - left);
	m[3][1] = -(top + bottom) / (top - bottom);
}

static GLuint compile_shader(unsigned type, unsigned count, const char **strings)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, count, strings, NULL);
	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		char buffer[4096];
		glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
	}

	return shader;
}

void init_shaders()
{
	GLuint vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
	GLuint fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);
	GLuint program = glCreateProgram();

	SDL_assert(program);

	glAttachShader(program, vshader);
	glAttachShader(program, fshader);
	glLinkProgram(program);

	glDeleteShader(vshader);
	glDeleteShader(fshader);

	glValidateProgram(program);

	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		char buffer[4096] = {0};
		glGetProgramInfoLog(program, sizeof(buffer), NULL, buffer);
	}

	g_video.g_shader.program = program;
	g_video.g_shader.i_pos = glGetAttribLocation(program, "i_pos");
	g_video.g_shader.i_coord = glGetAttribLocation(program, "i_coord");
	g_video.g_shader.u_tex = glGetUniformLocation(program, "u_tex");
	g_video.g_shader.u_mvp = glGetUniformLocation(program, "u_mvp");

	glGenVertexArrays(1, &g_video.g_shader.vao);
	glGenBuffers(1, &g_video.g_shader.vbo);

	glUseProgram(g_video.g_shader.program);

	float m[4][4] = {0};
	if (g_video.hw.bottom_left_origin)
		ortho2d(m, -1, 1, 1, -1);
	else
		ortho2d(m, -1, 1, -1, 1);

	glUniformMatrix4fv(g_video.g_shader.u_mvp, 1, GL_FALSE, (float *)m);

	glUseProgram(0);
}

void refresh_vertex_data()
{

	float bottom = (float)g_video.base_h / g_video.tex_h;
	float right = (float)g_video.base_w / g_video.tex_w;

	typedef struct
	{
		float x;
		float y;
		float s;
		float t;
	} vertex_data;

	vertex_data vert[] = {
		// pos, coord
		-1.0f, -1.0f, 0.0f, bottom, // left-bottom
		-1.0f, 1.0f, 0.0f, 0.0f,	// left-top
		1.0f, -1.0f, right, bottom, // right-bottom
		1.0f, 1.0f, right, 0.0f,	// right-top
	};

	glBindVertexArray(g_video.g_shader.vao);

	glBindBuffer(GL_ARRAY_BUFFER, g_video.g_shader.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data) * 4, vert, GL_STATIC_DRAW);

	glEnableVertexAttribArray(g_video.g_shader.i_pos);
	glEnableVertexAttribArray(g_video.g_shader.i_coord);
	glVertexAttribPointer(g_video.g_shader.i_pos, 2, GL_FLOAT, GL_FALSE,
						  sizeof(vertex_data), (void *)offsetof(vertex_data, x));
	glVertexAttribPointer(g_video.g_shader.i_coord, 2, GL_FLOAT, GL_FALSE,
						  sizeof(vertex_data), (void *)offsetof(vertex_data, s));

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

uintptr_t video_get_fb()
{
	return g_video.fbo_id;
}

bool video_sethw(struct retro_hw_render_callback *hw)
{
	hw->get_current_framebuffer = video_get_fb;
	hw->get_proc_address = (retro_hw_get_proc_address_t)SDL_GL_GetProcAddress;
	g_video.hw = *hw;
	return true;
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
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

bool video_init(const struct retro_game_geometry *geom, float &refreshrate, SDL_Window *context)
{
	g_video.g_shader = {0};

	g_video.software_rast = !g_video.hw.context_reset;
	g_video.sdl_context = context;
	init_shaders();

	if (g_video.tex_id)
		glDeleteTextures(1, &g_video.tex_id);

	g_video.tex_id = 0;

	if (!g_video.pixformat.pixfmt)
	{
		g_video.pixformat.pixfmt = GL_UNSIGNED_SHORT_1_5_5_5_REV;
		g_video.pixformat.pixtype = GL_BGRA;
		g_video.pixformat.bpp = sizeof(uint16_t);
	}
		

	glGenTextures(1, &g_video.tex_id);

	g_video.pitch = geom->base_width * g_video.pixformat.bpp;

	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
				 g_video.pixformat.pixtype, g_video.pixformat.pixfmt, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);

	init_framebuffer(geom->max_width, geom->max_height);

	g_video.tex_w = geom->max_width;
	g_video.tex_h = geom->max_height;
	g_video.base_w = geom->base_width;
	g_video.base_h = geom->base_height;
	g_video.aspect = geom->aspect_ratio;

	refresh_vertex_data();
	if (g_video.hw.context_reset)
		g_video.hw.context_reset();

	if (!g_video.rend_width)
		SDL_GetWindowSize((SDL_Window *)g_video.sdl_context, (int *)&g_video.rend_width, (int *)&g_video.rend_height);
	SDL_SetWindowPosition((SDL_Window *)g_video.sdl_context, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	return true;
}

void resize_cb()
{
	float aspect = g_video.aspect;
	if (aspect <= 0)
	{
		aspect = (float)g_video.base_w / (float)g_video.base_h;
	}
	unsigned height = g_video.rend_height;
	unsigned width = height * aspect;
	if (width > g_video.rend_width)
	{
		height = (float)g_video.rend_width / aspect;
		width = g_video.rend_width;
	}
	unsigned x = (g_video.rend_width - width) / 2;
	unsigned y = (g_video.rend_height - height) / 2;
	glViewport(x, y, width, height);
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
	refresh_vertex_data();
	resize_cb();
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glUseProgram(g_video.g_shader.program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
	glUniform1i(g_video.g_shader.u_tex, 0);
	glBindVertexArray(g_video.g_shader.vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GLenum error = glGetError();
	if (error != GL_NO_ERROR)
	{
		//   std::cerr << error << std::endl;
		return;
	}
	glBindVertexArray(0);
	glUseProgram(0);
}

void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch)
{
	if (g_video.base_w != width || g_video.base_h != height)
	{
		g_video.base_h = height;
		g_video.base_w = width;
	}
	if (data && data != RETRO_HW_FRAME_BUFFER_VALID)
	{
		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
		if (pitch != g_video.pitch)
			g_video.pitch = pitch;
		glPixelStorei(GL_UNPACK_ALIGNMENT, get_alignment(width * g_video.pixformat.bpp));
		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / g_video.pixformat.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixformat.pixtype,
						g_video.pixformat.pixfmt, data);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	}
}

void video_deinit()
{
	if (g_video.hw.context_destroy)
		g_video.hw.context_destroy();
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

	if (g_video.g_shader.vbo)
	{
		glDeleteBuffers(1, &g_video.g_shader.vbo);
		glDeleteVertexArrays(1, &g_video.g_shader.vao);
	}

	if (g_video.g_shader.program)
		glDeleteProgram(g_video.g_shader.program);
	if (g_video.g_shader.fshader)
		glDeleteShader(g_video.g_shader.fshader);
	if (g_video.g_shader.vshader)
		glDeleteShader(g_video.g_shader.vshader);
}