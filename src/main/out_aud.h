#ifndef outaud_h_
#define outaud_h_

#include <stdint.h>

void audio_mix(int16_t *samples, size_t size);
bool audio_init(float input_srate);
void audio_changerate(float input_srate);
void audio_destroy();
void audio_framelimit(bool cap);

#endif