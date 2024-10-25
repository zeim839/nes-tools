#ifndef NES_TOOLS_TRIANGLE_H
#define NES_TOOLS_TRIANGLE_H

#include "../system.h"
#include "audio.h"

typedef struct
{
	divider_t sequencer;
	uint8_t   length_counter;
	uint8_t   linear_reload;
	uint8_t   linear_counter;
	uint8_t   linear_reload_flag;
	uint8_t   halt;
	uint8_t   enabled;

} triangle_t;

extern const uint8_t triangle_sequence[32];

triangle_t triangle_create();
void triangle_set_counter(triangle_t* tri, uint8_t value);
void triangle_set_timer_low(triangle_t* tri, uint8_t value);
void triangle_set_length(triangle_t* tri, uint8_t value);
uint8_t triangle_clock(triangle_t* tri);

#endif // NES_TOOLS_TRIANGLE_H
