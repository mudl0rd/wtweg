#ifndef outaud_h_
#define outaud_h_

#include <stdint.h>

void audio_mix(void *samples, size_t size);
bool audio_init(double refreshra, float input_srate, double fps, bool fp);
void audio_changeratefps(double refreshra, float input_srate, double fps);
void audio_destroy();

#endif