#define THREADS_IMPLEMENTATION
#define RESAMPLER_IMPLEMENTATION
#include "resampler.h"
#include "rthreads.h"

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

