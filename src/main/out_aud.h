#ifndef outaud_h_
#define outaud_h_

#include <stdint.h>
#include <SDL2/SDL.h>

struct fifo_buffer
{
    uint8_t *buffer;
    size_t size;
    volatile int readpos;
    volatile int writepos;
};


#define fifo_size(x) ((x)->size - 1)
#define fifo_read_avail(x) (((x)->writepos - (x)->readpos) & fifo_size(x))
#define fifo_write_avail(x) ((x)->size - 1 - fifo_read_avail(x))

class out_aud
{
private:
    fifo_buffer *_fifo;
    SDL_AudioDeviceID dev;
    unsigned client_rate;
    double system_rate;
    void *resample;
    float *input_float;
    float *output_float;
    double drc_ratio;
    bool cap;
    static void buf_callback_( void*, Uint8*, int );
    void sound_cb( Uint8* out, int count );
    double resample_ratio (fifo_buffer * fifo);
public:
    ~out_aud();
    bool init(float input_srate);
    void changerate(float input_srate);
    void destroy();
    void framelimit(bool cap);
    void mix(int16_t* samples, size_t size);
};

#endif