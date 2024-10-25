#ifndef NES_TOOLS_NOISE_H
#define NES_TOOLS_NOISE_H

#include "../system.h"
#include "../mapper.h"
#include "audio.h"

typedef struct
{
	divider_t timer;
	uint8_t mode;
	uint8_t l;
	uint16_t shift;
	uint8_t const_volume;
	divider_t envelope;
	uint8_t envelope_loop;
	uint8_t enabled;

} noise_t;

noise_t noise_create();
void noise_set_ctrl(noise_t* noise, uint8_t val);
void noise_set_period(noise_t* noise, enum tv_system type, uint8_t val);
void noise_set_length(noise_t* noise, uint8_t val);

#endif // NES_TOOLS_NOISE_H
