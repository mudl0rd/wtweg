#define THREADS_IMPLEMENTATION
#define RESAMPLER_IMPLEMENTATION
#include <SDL2/SDL.h>
#include "resampler.h"
#include "rthreads.h"
#include "libretro.h"
#include "io.h"
#include "gl3w.h"

static const char* g_vshader_src =
"#version 330\n"
"in vec2 i_pos;\n"
"in vec2 i_coord;\n"
"out vec2 o_coord;\n"
"uniform mat4 u_mvp;\n"
"void main() {\n"
"o_coord = i_coord;\n"
"gl_Position = vec4(i_pos, 0.0, 1.0) * u_mvp;\n"
"}";

static const char* g_fshader_src = "#version 330\n"
"in vec2 o_coord;\n"
"uniform sampler2D u_tex;\n"
"void main() {\n"
"gl_FragColor = texture2D(u_tex, o_coord);\n"
"}";

#define FRAME_COUNT (1024)

struct fifo_buffer {
    uint8_t* buffer;
    size_t size;
    size_t first;
    size_t end;
};

struct audio_ctx {
    fifo_buffer* _fifo;
    SDL_AudioSpec shit;
    unsigned client_rate;
    double system_rate;
    void* resample;
    float* input_float;
    float* output_float;
    slock_t* cond_lock;
    scond_t* condz;
} audio_ctx_s;

typedef struct fifo_buffer fifo_buffer_t;

static __forceinline void fifo_clear(fifo_buffer_t* buffer) {
    buffer->first = 0;
    buffer->end = 0;
}

static __forceinline void fifo_free(fifo_buffer_t* buffer) {
    if (!buffer)
        return;

    free(buffer->buffer);
    free(buffer);
}

static __forceinline size_t fifo_read_avail(fifo_buffer_t* buffer) {
    return (buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
        buffer->first;
}

static __forceinline size_t fifo_write_avail(fifo_buffer_t* buffer) {
    return (buffer->size - 1) -
        ((buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
            buffer->first);
}

fifo_buffer_t* fifo_new(size_t size) {
    uint8_t* buffer = NULL;
    fifo_buffer_t* buf = (fifo_buffer_t*)calloc(1, sizeof(*buf));
    if (!buf)
        return NULL;
    buffer = (uint8_t*)calloc(1, size + 1);
    if (!buffer) {
        free(buf);
        return NULL;
    }
    buf->buffer = buffer;
    buf->size = size + 1;
    return buf;
}

void fifo_write(fifo_buffer_t* buffer, const void* in_buf, size_t size) {
    size_t first_write = size;
    size_t rest_write = 0;
    if (buffer->end + size > buffer->size) {
        first_write = buffer->size - buffer->end;
        rest_write = size - first_write;
    }
    memcpy(buffer->buffer + buffer->end, in_buf, first_write);
    memcpy(buffer->buffer, (const uint8_t*)in_buf + first_write, rest_write);
    buffer->end = (buffer->end + size) % buffer->size;
}

void fifo_read(fifo_buffer_t* buffer, void* in_buf, size_t size) {
    size_t first_read = size;
    size_t rest_read = 0;
    if (buffer->first + size > buffer->size) {
        first_read = buffer->size - buffer->first;
        rest_read = size - first_read;
    }
    memcpy(in_buf, (const uint8_t*)buffer->buffer + buffer->first, first_read);
    memcpy((uint8_t*)in_buf + first_read, buffer->buffer, rest_read);
    buffer->first = (buffer->first + size) % buffer->size;
}

__forceinline void s16tof(float* dst, const int16_t* src, unsigned int count)
{
    int i = 0;
    float fgain = 1.0 / UINT32_C(0x80000000);
    __m128 factor = _mm_set1_ps(fgain);
    for (i = 0; i + 8 <= count; i += 8, src += 8, dst += 8)
    {
        __m128i input = _mm_loadu_si128((const __m128i*)src);
        __m128i regs_l = _mm_unpacklo_epi16(_mm_setzero_si128(), input);
        __m128i regs_r = _mm_unpackhi_epi16(_mm_setzero_si128(), input);
        __m128 output_l = _mm_mul_ps(_mm_cvtepi32_ps(regs_l), factor);
        __m128 output_r = _mm_mul_ps(_mm_cvtepi32_ps(regs_r), factor);
        _mm_storeu_ps(dst + 0, output_l);
        _mm_storeu_ps(dst + 4, output_r);
    }
    fgain = 1.0/ 0x8000;
    count = count - i;
    i = 0;
    for (; i < count; i++)
        dst[i] = (float)src[i] * fgain;
}

void func_callback(void* userdata, Uint8* stream, int len)
{
    audio_ctx* context = (audio_ctx*)userdata;
    size_t amount = fifo_read_avail(context->_fifo);
    amount = (len >= amount) ? amount : len;
    fifo_read(context->_fifo, (uint8_t*)stream, amount);
    memset(stream + amount, 0, len - amount);
    scond_signal(context->condz);
}

void audio_mix(const int16_t* samples, size_t size) {
    struct resampler_data src_data = { 0 };
    size_t written = 0;
    uint32_t in_len = size * 2;
    double maxdelta = 0.005;
    auto bufferlevel = []() {
        return double(
            (audio_ctx_s._fifo->size - (int)fifo_write_avail(audio_ctx_s._fifo)) /
            audio_ctx_s._fifo->size);
    };
    int newInputFrequency =
        ((1.0 - maxdelta) + 2.0 * (double)bufferlevel() * maxdelta) *
        audio_ctx_s.system_rate;
    float drc_ratio = (float)audio_ctx_s.client_rate / (float)newInputFrequency;
    s16tof(audio_ctx_s.input_float, samples, in_len);
    src_data.input_frames = size;
    src_data.ratio = drc_ratio;
    src_data.data_in = audio_ctx_s.input_float;
    src_data.data_out = audio_ctx_s.output_float;
    resampler_sinc_process(audio_ctx_s.resample, &src_data);
    int out_len = src_data.output_frames * 2 * sizeof(float);
    while (written < out_len) {
        size_t avail = fifo_write_avail(audio_ctx_s._fifo);
        if (avail) {
        
            size_t write_amt = out_len - written > avail ? avail : out_len - written;
            fifo_write(audio_ctx_s._fifo,
                (const char*)audio_ctx_s.output_float + written, write_amt);
            written += write_amt;
        }
        else {
            slock_lock(audio_ctx_s.cond_lock);
            scond_wait(audio_ctx_s.condz, audio_ctx_s.cond_lock);
        }
    }
}


bool audio_init(double refreshra, float input_srate, float fps) {
    audio_ctx_s.cond_lock = slock_new();
    audio_ctx_s.condz = scond_new();
    audio_ctx_s.system_rate = input_srate;
    double system_fps = fps;
    if (fabs(1.0f - system_fps / refreshra) <= 0.05)
        audio_ctx_s.system_rate *= (refreshra / system_fps);

    audio_ctx_s.shit.freq = 44100;
    audio_ctx_s.shit.format = AUDIO_F32;
    audio_ctx_s.shit.samples = FRAME_COUNT;
    audio_ctx_s.shit.callback= func_callback;
    audio_ctx_s.shit.userdata= (audio_ctx*)&audio_ctx_s;
    audio_ctx_s.shit.channels = 2;
    audio_ctx_s.client_rate = audio_ctx_s.shit.freq;
    audio_ctx_s.resample = resampler_sinc_init();
    // allow for tons of space in the tank
    size_t outsamples_max = (FRAME_COUNT * 4 * sizeof(float));
    size_t sampsize = (FRAME_COUNT * (2 * sizeof(float)));
    audio_ctx_s._fifo = fifo_new(sampsize); // number of bytes
    audio_ctx_s.output_float =
        new float[outsamples_max]; // spare space for resampler
    audio_ctx_s.input_float = new float[outsamples_max];

    uint8_t* tmp = (uint8_t*)calloc(1, sampsize);
    if (tmp) {
        fifo_write(audio_ctx_s._fifo, tmp, sampsize);
        free(tmp);
    }

    SDL_OpenAudio(&audio_ctx_s.shit, NULL);
    SDL_PauseAudio(0);
    return true;
}
void audio_destroy() {
    {
        SDL_PauseAudio(0);
        SDL_CloseAudio();
        fifo_free(audio_ctx_s._fifo);
        scond_free(audio_ctx_s.condz);
        slock_free(audio_ctx_s.cond_lock);
        delete[] audio_ctx_s.input_float;
        delete[] audio_ctx_s.output_float;
        resampler_sinc_free(audio_ctx_s.resample);
    }
}

class gl_render : public libretro_render
{
private:
	struct {
		GLuint vao;
		GLuint vbo;
		GLuint program;
		GLuint fshader;
		GLuint vshader;
		GLint i_pos;
		GLint i_coord;
		GLint u_tex;
		GLint u_mvp;
	} g_shader;


    void *sdl_context;
	int rend_width;
	int rend_height;

	struct {
		GLuint tex_id;
		GLuint rbo_id;

		int glmajor;
		int glminor;
		int last_h;
		int last_w;
		bool software_rast;
		GLuint pitch;
		bool sw_core;
		GLint tex_w, tex_h;
		GLuint base_w, base_h;
		float aspect;
		int aspect_factor;
		GLuint pixfmt;
		GLuint pixtype;
		GLuint bpp;
		struct retro_hw_render_callback hw;
	}g_video;

	void ortho2d(float m[4][4], float left, float right, float bottom, float top) {
		m[3][3] = 1;
		m[0][0] = 2.0f / (right - left);
		m[1][1] = 2.0f / (top - bottom);
		m[2][2] = -1.0f;
		m[3][0] = -(right + left) / (right - left);
		m[3][1] = -(top + bottom) / (top - bottom);
	}

	GLuint compile_shader(unsigned type, unsigned count, const char** strings) {
		GLint status;
		GLuint shader = glCreateShaderProgramv(type, 1, strings);
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status == GL_FALSE) {
			char buffer[4096];
			glGetShaderInfoLog(shader, sizeof(buffer), NULL, buffer);
		}
		return shader;
	}

	void init_shaders() {
		GLint status;
		g_shader.vshader = compile_shader(GL_VERTEX_SHADER, 1, &g_vshader_src);
		g_shader.fshader = compile_shader(GL_FRAGMENT_SHADER, 1, &g_fshader_src);

		glGenProgramPipelines(1, &g_shader.program);
		glBindProgramPipeline(g_shader.program);
		glUseProgramStages(g_shader.program, GL_VERTEX_SHADER_BIT, g_shader.vshader);
		glUseProgramStages(g_shader.program, GL_FRAGMENT_SHADER_BIT,
			g_shader.fshader);

		glGetProgramiv(g_shader.program, GL_LINK_STATUS, &status);
		if (status == GL_FALSE) {
			char buffer[4096] = { 0 };
			glGetProgramInfoLog(g_shader.program, sizeof(buffer), NULL, buffer);
		}
		g_shader.i_pos = glGetAttribLocation(g_shader.vshader, "i_pos");
		g_shader.i_coord = glGetAttribLocation(g_shader.vshader, "i_coord");
		g_shader.u_tex = glGetUniformLocation(g_shader.fshader, "u_tex");
		g_shader.u_mvp = glGetUniformLocation(g_shader.vshader, "u_mvp");

		glGenVertexArrays(1, &g_shader.vao);
		glGenBuffers(1, &g_shader.vbo);
		glProgramUniform1i(g_shader.fshader, g_shader.u_tex, 0);

		float m[4][4] = { 0 };
		ortho2d(m, -1, 1, 1, -1);
		glProgramUniformMatrix4fv(g_shader.vshader, g_shader.u_mvp, 1, GL_FALSE,
			(float*)m);
		glBindProgramPipeline(0);
	}

	void refresh_vertex_data() {

		float bottom = (float)g_video.base_h / g_video.tex_h;
		float right = (float)g_video.base_w / g_video.tex_w;

		typedef struct {
			float x;
			float y;
			float s;
			float t;
		} vertex_data;

		vertex_data vert[] = {
			// pos, coord
			-1.0f, -1.0f,  0.0f,  bottom, // left-bottom
			-1.0f, 1.0f,  0.0f,  0.0f,   // left-top
			1.0f,  -1.0f, right, bottom, // right-bottom
			1.0f,  1.0f, right, 0.0f,   // right-top
		};

		glBindVertexArray(g_shader.vao);
		glBindBuffer(GL_ARRAY_BUFFER, g_shader.vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data) * 4, vert, GL_STATIC_DRAW);
		glEnableVertexAttribArray(g_shader.i_pos);
		glEnableVertexAttribArray(g_shader.i_coord);
		glVertexAttribPointer(g_shader.i_pos, 2, GL_FLOAT, GL_FALSE,
			sizeof(vertex_data), (void*)offsetof(vertex_data, x));
		glVertexAttribPointer(g_shader.i_coord, 2, GL_FLOAT, GL_FALSE,
			sizeof(vertex_data), (void*)offsetof(vertex_data, s));
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}


	void init_framebuffer(int width, int height) {
		if (fbo_id)
			glDeleteFramebuffers(1, &fbo_id);
		if (g_video.rbo_id)
			glDeleteRenderbuffers(1, &g_video.rbo_id);

		glGenFramebuffers(1, &fbo_id);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_id);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
			g_video.tex_id, 0);
		if (g_video.hw.depth) {
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

	void resize_cb() {
		int pad_x = 0, pad_y = 0;
		if (!rend_width || !rend_height)
			return;
		int rend_width = this->rend_width;
		int rend_height = this->rend_height;
		float screenaspect = rend_width / rend_height;
		unsigned base_h = g_video.base_h;
		if (base_h == 0)base_h = 1;
		unsigned base_w = (unsigned)roundf(base_h * screenaspect);
		if (rend_width >= base_w && rend_height >= base_h)
		{
			unsigned scale = (unsigned)fminf(rend_width / base_w, rend_height / base_h);
			pad_x = rend_width - base_w * scale;
			pad_y = rend_height - base_h * scale;
		}
		rend_width -= pad_x;
		rend_height -= pad_y;
		glViewport(pad_x / 2, pad_y / 2, rend_width, rend_height);
	}

	void create_window(int width, int height) {
		init_shaders();
		g_video.last_w = 0;
		g_video.last_h = 0;
		rend_width = width;
		rend_height = height;
	}

	void resize_to_aspect(double ratio, int sw, int sh, int* dw, int* dh) {
		*dw = sw;
		*dh = sh;

		if (ratio <= 0)
			ratio = (double)sw / sh;

		if ((float)sw / sh < 1)
			*dw = *dh * ratio;
		else
			*dh = *dw / ratio;
	}

	bool set_pixelformat(retro_pixel_format fmt) {
		switch (fmt) {
		case RETRO_PIXEL_FORMAT_0RGB1555:
			g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
			g_video.pixtype = GL_BGRA;
			g_video.bpp = sizeof(uint16_t);
			break;
		case RETRO_PIXEL_FORMAT_XRGB8888:
			g_video.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
			g_video.pixtype = GL_BGRA;
			g_video.bpp = sizeof(uint32_t);
			break;
		case RETRO_PIXEL_FORMAT_RGB565:
			g_video.pixfmt = GL_UNSIGNED_SHORT_5_6_5;
			g_video.pixtype = GL_RGB;
			g_video.bpp = sizeof(uint16_t);
			break;
		default:
			break;
		}
		return true;
	}

public:
	GLuint fbo_id;

	uintptr_t get_fb()
	{
		return fbo_id;
	}

	gl_render(void *context) {
		g_video = { 0 };
		g_shader = { 0 };
		g_video.hw.version_major = 4;
		g_video.hw.version_minor = 5;
		g_video.hw.context_type = RETRO_HW_CONTEXT_NONE;
		g_video.hw.context_reset = NULL;
		g_video.hw.context_destroy = NULL;
		g_video.software_rast = true;
		fbo_id = 0;
		sdl_context = context;
	}
	~gl_render() {
		deinit();
	}

	void set_hwcontext(retro_hw_render_callback* hw) {
		hw->get_proc_address = (retro_hw_get_proc_address_t)gl3wGetProcAddress;
		g_video.hw = *hw;
		g_video.software_rast = false;
	}

	bool init(const struct retro_game_geometry* geom, retro_pixel_format fmt, float& refreshrate)
	{
		int width = 0, height = 0;

		resize_to_aspect(geom->aspect_ratio, geom->base_width * 1,
			geom->base_height * 1, &width, &height);

		width *= 1;
		height *= 1;

		create_window(width, height);

		if (g_video.tex_id)
			glDeleteTextures(1, &g_video.tex_id);

		g_video.tex_id = 0;

		if (!g_video.pixfmt)
			g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;

		glGenTextures(1, &g_video.tex_id);

		g_video.pitch = geom->base_width * g_video.bpp;

		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

		set_pixelformat(fmt);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
			g_video.pixtype, g_video.pixfmt, NULL);

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

        SDL_DisplayMode dm;
        SDL_GetDesktopDisplayMode(0, &dm);
		double x = (double)dm.w / (double)geom->base_width;
		double y = (double)dm.h / (double)geom->base_height;
		double factor = x < y ? x : y;
		int int_factor = unsigned(factor);
		int nominal = int_factor;
		SDL_SetWindowSize((SDL_Window*)sdl_context, g_video.base_w * nominal,
			g_video.base_h * nominal);
        SDL_SetWindowPosition((SDL_Window*)sdl_context,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
		return true;
	}

	virtual void resize(int rend_width, int rend_height)
	{
		this->rend_width = rend_width;
		this->rend_height = rend_height;
	}

	void refresh(const void* data, unsigned width, unsigned height, unsigned pitch)
    {
		if (data == NULL)
			return;
		if (g_video.base_w != width || g_video.base_h != height) {
			g_video.base_h = height;
			g_video.base_w = width;

			refresh_vertex_data();
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		resize_cb();
		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);

		if (data && data != RETRO_HW_FRAME_BUFFER_VALID) {
			g_video.pitch = pitch;
			glPixelStorei(GL_UNPACK_ROW_LENGTH, g_video.pitch / g_video.bpp);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, g_video.pixtype,
				g_video.pixfmt, data);
		}

		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(0);
		glBindProgramPipeline(g_shader.program);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, g_video.tex_id);
		glProgramUniform1i(g_shader.fshader, g_shader.u_tex, 0);
		glBindVertexArray(g_shader.vao);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glBindProgramPipeline(0);
	}

	
	void deinit()
	{
		if (g_video.tex_id) {
			glDeleteTextures(1, &g_video.tex_id);
			g_video.tex_id = 0;
		}
		if (fbo_id) {
			glDeleteFramebuffers(1, &fbo_id);
			fbo_id = 0;
		}

		if (g_video.rbo_id) {
			glDeleteRenderbuffers(1, &g_video.rbo_id);
			g_video.rbo_id = 0;
		}
		
		if (g_shader.vbo)
		{
			glDisableVertexAttribArray(1);
			glDeleteBuffers(1, &g_shader.vbo);
			glDeleteVertexArrays(1, &g_shader.vbo);
		}
		
		if(g_shader.program)
		glDeleteProgramPipelines(1, &g_shader.program);
		if (g_shader.fshader)
		glDeleteShader(g_shader.fshader);
		if (g_shader.vshader)
		glDeleteShader(g_shader.vshader);
	}

	void buf_clear()
	{
		if (!g_video.software_rast) {
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
	}

	

};

libretro_render* create_gl_render(void *context)
{
	return new gl_render(context);
}