#ifndef NES_TOOLS_PULSE_H
#define NES_TOOLS_PULSE_H

#include "audio.h"

typedef struct
{
	divider_t t;
	uint8_t   l;
	uint8_t   id;
	uint8_t   neg;
	uint8_t   shift;
	divider_t sweep;
	uint8_t   enable_sweep;
	uint8_t   duty;
	uint8_t   const_volume;
	divider_t envelope;
	uint8_t   envelope_loop;
	uint8_t   enabled;
	uint8_t   mute;
	uint16_t  target_period;

} pulse_t;

pulse_t pulse_create(uint8_t id);
void pulse_set_ctrl(pulse_t* pulse, uint8_t value);
void pulse_set_timer(pulse_t* pulse, uint8_t value);
void pulse_set_sweep(pulse_t* pulse, uint8_t value);
void pulse_set_length_counter(pulse_t* pulse, uint8_t value);
void pulse_length_sweep(pulse_t* pulse);

#endif // NES_TOOLS_PULSE_H
