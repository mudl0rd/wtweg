#define THREADS_IMPLEMENTATION
#define RESAMPLER_IMPLEMENTATION
#include <SDL2/SDL.h>
#include "resampler.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include "libretro.h"
#include "io.h"

#define FRAME_COUNT (1024)

struct fifo_buffer
{
    uint8_t *buffer;
    size_t size;
    size_t first;
    size_t end;
};

struct audio_ctx
{
    fifo_buffer *_fifo;
    SDL_AudioSpec shit;
    unsigned client_rate;
    double system_rate;
    void *resample;
    std::mutex mutex;
    std::condition_variable cv;
    bool processed;
} audio_ctx_s;

typedef struct fifo_buffer fifo_buffer_t;

static inline void fifo_clear(fifo_buffer_t *buffer)
{
    buffer->first = 0;
    buffer->end = 0;
}

static inline void fifo_free(fifo_buffer_t *buffer)
{
    if (!buffer)
        return;

    free(buffer->buffer);
    free(buffer);
}

static inline size_t fifo_read_avail(fifo_buffer_t *buffer)
{
    return (buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
           buffer->first;
}

static inline size_t fifo_write_avail(fifo_buffer_t *buffer)
{
    return (buffer->size - 1) -
           ((buffer->end + ((buffer->end < buffer->first) ? buffer->size : 0)) -
            buffer->first);
}

fifo_buffer_t *fifo_new(size_t size)
{
    uint8_t *buffer = NULL;
    fifo_buffer_t *buf = (fifo_buffer_t *)calloc(1, sizeof(*buf));
    if (!buf)
        return NULL;
    buffer = (uint8_t *)calloc(1, size + 1);
    if (!buffer)
    {
        free(buf);
        return NULL;
    }
    buf->buffer = buffer;
    buf->size = size + 1;
    return buf;
}

void fifo_write(fifo_buffer_t *buffer, const void *in_buf, size_t size)
{
    size_t first_write = size;
    size_t rest_write = 0;
    if (buffer->end + size > buffer->size)
    {
        first_write = buffer->size - buffer->end;
        rest_write = size - first_write;
    }
    memcpy(buffer->buffer + buffer->end, in_buf, first_write);
    memcpy(buffer->buffer, (const uint8_t *)in_buf + first_write, rest_write);
    buffer->end = (buffer->end + size) % buffer->size;
}

void fifo_read(fifo_buffer_t *buffer, void *in_buf, size_t size)
{
    size_t first_read = size;
    size_t rest_read = 0;
    if (buffer->first + size > buffer->size)
    {
        first_read = buffer->size - buffer->first;
        rest_read = size - first_read;
    }
    memcpy(in_buf, (const uint8_t *)buffer->buffer + buffer->first, first_read);
    memcpy((uint8_t *)in_buf + first_read, buffer->buffer, rest_read);
    buffer->first = (buffer->first + size) % buffer->size;
}

inline void s16tof(float *dst, const int16_t *src, unsigned int count)
{
    unsigned int i = 0;
    float fgain = 1.0 / UINT32_C(0x80000000);
    __m128 factor = _mm_set1_ps(fgain);
    for (i = 0; i + 8 <= count; i += 8, src += 8, dst += 8)
    {
        __m128i input = _mm_loadu_si128((const __m128i *)src);
        __m128i regs_l = _mm_unpacklo_epi16(_mm_setzero_si128(), input);
        __m128i regs_r = _mm_unpackhi_epi16(_mm_setzero_si128(), input);
        __m128 output_l = _mm_mul_ps(_mm_cvtepi32_ps(regs_l), factor);
        __m128 output_r = _mm_mul_ps(_mm_cvtepi32_ps(regs_r), factor);
        _mm_storeu_ps(dst + 0, output_l);
        _mm_storeu_ps(dst + 4, output_r);
    }
    fgain = 1.0 / 0x8000;
    count = count - i;
    i = 0;
    for (; i < count; i++)
        dst[i] = (float)src[i] * fgain;
}

void func_callback(void *userdata, Uint8 *stream, int len)
{
    audio_ctx *context = (audio_ctx *)userdata;
    std::unique_lock<std::mutex> lk(context->mutex);
    int amount = fifo_read_avail(context->_fifo);
    amount = (len > amount) ? amount : len;
    fifo_read(context->_fifo, (uint8_t *)stream, amount);
    memset(stream + amount, 0, len - amount);
}

void audio_mix(const int16_t *samples, size_t size)
{
    struct resampler_data src_data = {0};
    size_t written = 0;
    uint32_t in_len = size * 2;
    double maxdelta = 0.005;
    auto bufferlevel = []()
    {
        return double(
            (audio_ctx_s._fifo->size - (int)fifo_write_avail(audio_ctx_s._fifo)) /
            audio_ctx_s._fifo->size);
    };
    int newInputFrequency =
        ((1.0 - maxdelta) + 2.0 * (double)bufferlevel() * maxdelta) *
        audio_ctx_s.system_rate;
    float drc_ratio = (float)audio_ctx_s.client_rate / (float)newInputFrequency;

    auto input_float = std::make_unique<float[]>(in_len);
    auto output_float = std::make_unique<float[]>(in_len * 4);

    s16tof(input_float.get(), samples, in_len);
    src_data.input_frames = size;
    src_data.ratio = drc_ratio;
    src_data.data_in = input_float.get();
    src_data.data_out = output_float.get();
    resampler_sinc_process(audio_ctx_s.resample, &src_data);
    size_t out_len = src_data.output_frames * 2 * sizeof(float);
    audio_ctx_s.processed = false;
    while (written < out_len)
    {
        SDL_LockAudio();
        size_t avail = fifo_write_avail(audio_ctx_s._fifo);
        if (avail)
        {

            size_t write_amt = out_len - written > avail ? avail : out_len - written;
            fifo_write(audio_ctx_s._fifo,
                       (const char *)output_float.get() + written, write_amt);
            SDL_UnlockAudio();
            written += write_amt;
        }
        else
        {
            SDL_UnlockAudio();
        }
    }
}

unsigned long upper_power_of_two(unsigned long v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;

}

bool audio_init(double refreshra, float input_srate, float fps)
{

  int swap =1;
  if ((int)refreshra% (int)fps == 0) {
    swap = refreshra / fps;
  }
  float timing_skew = fabs(1.0f - fps / (refreshra / (float)swap));
  float target_video_sync_rate = refreshra/ (float) swap;
   if (timing_skew <= 0.05)
       audio_ctx_s.system_rate= input_srate * target_video_sync_rate / fps;


    int frames = upper_power_of_two((44100 * 60) / 1000);

    audio_ctx_s.processed = true;
    audio_ctx_s.system_rate = input_srate;
    double system_fps = fps;
    audio_ctx_s.shit.freq = 44100;
    audio_ctx_s.shit.format = AUDIO_F32;
    audio_ctx_s.shit.samples = frames;
    audio_ctx_s.shit.callback = func_callback;
    audio_ctx_s.shit.userdata = (audio_ctx *)&audio_ctx_s;
    audio_ctx_s.shit.channels = 2;
    audio_ctx_s.client_rate = audio_ctx_s.shit.freq;
    audio_ctx_s.resample = resampler_sinc_init();
    // allow for tons of space in the tank
    SDL_AudioSpec out;
    SDL_OpenAudio(&audio_ctx_s.shit, &out);
   
    size_t sampsize = (out.samples * (4 * sizeof(float)));
    audio_ctx_s._fifo = fifo_new(sampsize); // number of bytes
    auto tmp = std::make_unique<uint8_t[]>(sampsize);
    fifo_write(audio_ctx_s._fifo, tmp.get(), sampsize);
    SDL_PauseAudio(0);
    return true;
}
void audio_destroy()
{
    {
        SDL_PauseAudio(1);
        SDL_CloseAudio();
        fifo_free(audio_ctx_s._fifo);
        resampler_sinc_free(audio_ctx_s.resample);
    }
}