#ifndef NES_TOOLS_AUDIO_H
#define NES_TOOLS_AUDIO_H

#include "../system.h"
#include "../mapper.h"

#define SAMPLING_FREQUENCY   48000
#define AUDIO_BUFF_SIZE      1024
#define STATS_WIN_SIZE       20
#define AVERAGE_DOWNSAMPLING 0
#define NOMINAL_QUEUE_SIZE   6000

enum
{
	TIMER_HIGH = 0x7,
	PULSE_SHIFT = 0x7,
	PULSE_PERIOD = 0b01110000,
};

typedef struct
{
	long long period;
	long long counter;
	uint32_t  step;
	uint32_t  limit;
	uint32_t  from;
	uint8_t   loop;

} divider_t;

typedef struct
{
	uint16_t factor_index;
	uint16_t target_factor;
	uint16_t equilibrium_factor;
	uint16_t max_factor;
	size_t   samples;
	size_t   max_period;
	size_t   min_period;
	size_t   period;
	size_t   counter;
	size_t   index;
	size_t   max_index;

} sampler_t;

extern const uint8_t length_counter_lookup[32];

uint8_t divider_clock(divider_t *divider);
uint8_t divider_clock_inverse(divider_t* divider);

#endif // NES_TOOLS_AUDIO_H
