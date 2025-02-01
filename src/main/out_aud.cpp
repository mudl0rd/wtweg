#define RESAMPLER_IMPLEMENTATION
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include "resampler.h"
#include "libretro.h"
#include "clibretro.h"
#include "mudutils/utils.h"
#include <thread>
#include <mutex>
#include <condition_variable>

typedef struct fifo_buffer fifo_buffer_t;

static inline void fifo_clear(fifo_buffer_t *buffer)
{
    buffer->readpos = 0;
    buffer->writepos = 0;
}

void fifo_free(fifo_buffer_t *buffer)
{
    if (!buffer)
        return;

    free(buffer->buffer);
    free(buffer);
}

fifo_buffer_t *fifo_new(size_t size)
{
    uint8_t *buffer = NULL;
    fifo_buffer_t *buf = (fifo_buffer_t *)calloc(1, sizeof(*buf));
    if (!buf)
        return NULL;
    buf->size = size;
    buffer = (uint8_t *)malloc(size);
    if (!buffer)
    {
        free(buf);
        return NULL;
    }
    memset(buffer, 0, size);
    buf->buffer = buffer;
    buf->size = size;
    return buf;
}

int inline fifo_write(fifo_buffer_t *buffer, void *in_buf, size_t size, bool read)
{

    int total;
    int i;
    char *buf = (char *)in_buf;
    total = (read) ? fifo_read_avail(buffer) : fifo_write_avail(buffer);
    if (size > total)
        size = total;
    else
        total = size;
    i = (read) ? buffer->readpos : buffer->writepos;
    if (i + size > buffer->size)
    {
        if (read)
            memcpy(buf, buffer->buffer + i, buffer->size - i);
        else
            memcpy(buffer->buffer + i, buf, buffer->size - i);
        buf += buffer->size - i;
        size -= buffer->size - i;
        i = 0;
    }
    if (read)
    {
        memcpy(buf, buffer->buffer + i, size);
        buffer->readpos = i + size;
    }
    else
    {
        memcpy(buffer->buffer + i, buf, size);
        buffer->writepos = i + size;
    }
    return total;
}

int fifo_writespin(fifo_buffer_t *f, void *buf, unsigned len)
{
    while (fifo_write_avail(f) < len)
        ;
    return fifo_write(f, buf, len, false);
}

int fifo_readspin(fifo_buffer_t *f, void *buf, unsigned len)
{
    while (fifo_read_avail(f) < len)
        ;
    return fifo_write(f, buf, len, true);
}

unsigned int
mix_sample_buffer(
    const float volume,
    unsigned int num_frames,
    const float *src,
    float *dst)
{
#define fmin -1.0f
#define fmax 1.0f
    while (num_frames)
    {
        float s = *src;
        float d = *dst;
        float m = s * volume;
        float a = d + m;
        d = a > fmax ? fmax : a < fmin ? fmin
                                       : a;
        *dst = d;
        dst++;
        src++;
        num_frames--;
    }
    return num_frames;
}

out_aud::~out_aud()
{
}
bool out_aud::init(float input_srate)
{
    SDL_AudioSpec shit = {0};
    SDL_AudioSpec shit2 = {0};
    cap = true;
    system_rate = input_srate;
    SDL_GetDefaultAudioInfo(NULL, &shit2, 0);
    auto desired_samples = (64 * shit2.freq) / 1000.0f;
    shit.samples = MudUtil::pow2up(desired_samples); // SDL2 requires power-of-two buffer sizes
    shit.freq = shit2.freq;
    shit.format = AUDIO_F32;
    shit.callback = buf_callback_;
    shit.userdata = this;
    shit.channels = 2;
    client_rate = shit.freq;
    resample = resampler_sinc_init();
    SDL_AudioSpec out;
    dev = SDL_OpenAudioDevice(NULL, 0, &shit, &out, 0);
    size_t sampsize = out.size * 2;
    input_float = (float *)memalign_alloc(64, sampsize);
    output_float = (float *)memalign_alloc(64, sampsize);
    memset(input_float, 0, sampsize);
    memset(output_float, 0, sampsize);
    _fifo = fifo_new(sampsize); // number of bytes
    SDL_PauseAudioDevice(dev, 0);
    return false;
}
void out_aud::changerate(float input_srate)
{
    system_rate = input_srate;
}
void out_aud::destroy()
{
    SDL_PauseAudioDevice(dev, 1);
    SDL_CloseAudioDevice(dev);
    fifo_free(_fifo);
    memalign_free(input_float);
    memalign_free(output_float);
    resampler_sinc_free(resample);
}
void out_aud::framelimit(bool cap)
{
    this->cap = cap;
}

void out_aud::buf_callback_(void *user_data, Uint8 *out, int count)
{
    ((out_aud *)user_data)->sound_cb(out, count);
}

void out_aud::sound_cb(Uint8 *out, int bytes)
{
    if (cap)
    {
        int amount = fifo_read_avail(_fifo);
        amount = (bytes > amount) ? amount : bytes;
        fifo_write(_fifo, (uint8_t *)out, amount, true);
        memset(out + amount, 0, bytes - amount);
    }
    else
        memset(out, 0, bytes);
}

double out_aud::resample_ratio(fifo_buffer *buf)
{
    double maxdelta = 0.005;
    auto bufferlevel = [=]()
    {
        return double(
            (buf->size - (int)fifo_write_avail(buf)) /
            buf->size);
    };
    double freq =
        ((1.0 - maxdelta) + 2.0 * (double)bufferlevel() * maxdelta) *
        system_rate;
    return (double)client_rate / freq;
}

void out_aud::mix(int16_t *samples, size_t size)
{
    if (cap)
    {

        struct resampler_data src_data = {0};
        size_t written = 0;
        uint32_t in_len = size * 2;

        while (in_len--)
            input_float[in_len] = (float)samples[in_len] * 0.000030517578125f;
        src_data.data_in = input_float;
        src_data.input_frames = size;
        src_data.ratio = resample_ratio(_fifo);
        src_data.data_out = output_float;
        resampler_sinc_process(resample, &src_data);
        size_t out_bytes = src_data.output_frames * 2 * sizeof(float);

        while (written < out_bytes)
        {

            size_t avail = fifo_write_avail(_fifo);
            if (avail)
            {
                SDL_LockAudioDevice(dev);
                size_t write_amt = out_bytes - written > avail ? avail : out_bytes - written;
                fifo_write(_fifo,
                           (char *)output_float + written, write_amt, false);
                written += write_amt;
                SDL_UnlockAudioDevice(dev);
            }
        }
    }
}