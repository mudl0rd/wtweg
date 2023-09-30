
#include <SDL2/SDL.h>
#include "libretro.h"
#include "io.h"
#include "mudutils/utils.h"

struct audio_ctx
{
    SDL_AudioStream *stream;
    SDL_AudioDeviceID dev;
    unsigned client_rate;
    double system_rate;
    float *input_float;
    float *output_float;
} audio_ctx_s;


void func_callback(void *userdata, Uint8 *stream, int len)
{
    audio_ctx *context = (audio_ctx *)userdata;
    int amount = SDL_AudioStreamAvailable(context->stream);
    amount = (len > amount) ? amount : len;
    int get = SDL_AudioStreamGet(context->stream, stream, (int)amount);
    memset(stream + amount, 0, len - get);
}

void audio_mix(const int16_t *samples, size_t size)
{
    size_t written = 0;
    uint32_t in_len = size * 2;
    uint32_t out_bytes = size*2*sizeof(int16_t);
    SDL_LockAudioDevice(audio_ctx_s.dev);
    SDL_AudioStreamPut(audio_ctx_s.stream, samples+written, out_bytes);
    SDL_UnlockAudioDevice(audio_ctx_s.dev);
}

void audio_changeratefps(float refreshra, float input_srate, float fps)
{
    unsigned swap = 1;
    if (refreshra > fps)
        swap = refreshra / (unsigned)fps;
    float refreshtarget = refreshra / swap;
    float timing_skew = fabs(1.0f - fps / refreshtarget);
    if (timing_skew <= 0.05)
        audio_ctx_s.system_rate = input_srate * (refreshtarget / fps);
    else
        audio_ctx_s.system_rate = input_srate;
}

bool audio_init(float refreshra, float input_srate, float fps)
{
    SDL_AudioSpec shit = {0};
    audio_changeratefps(refreshra, input_srate, fps);
    SDL_AudioSpec shit2 = {0};
    SDL_GetDefaultAudioInfo(NULL, &shit2, 0);
    shit.freq = shit2.freq;
    shit.format = AUDIO_F32;
    shit.samples = 2048;
    shit.callback = func_callback;
    shit.userdata = (audio_ctx *)&audio_ctx_s;
    shit.channels = 2;
    audio_ctx_s.client_rate = shit.freq;
    SDL_AudioSpec out;
    audio_ctx_s.dev = SDL_OpenAudioDevice(NULL, 0, &shit, &out, 0);
    audio_ctx_s.stream = SDL_NewAudioStream(AUDIO_S16SYS, 2, audio_ctx_s.system_rate,
     AUDIO_F32SYS, 2, audio_ctx_s.client_rate);
    SDL_PauseAudioDevice(audio_ctx_s.dev, 0);
    return true;
}
void audio_destroy()
{
    SDL_PauseAudioDevice(audio_ctx_s.dev, 1);
    SDL_CloseAudioDevice(audio_ctx_s.dev);
    SDL_FreeAudioStream(audio_ctx_s.stream);
}