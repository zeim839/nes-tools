#include "pulse.h"

static void update_target_period(pulse_t* pulse)
{
	int change = pulse->t.period >> pulse->shift;
	change = pulse->neg ? pulse->id == 1 ? - change - 1 : -change : change;

	// Add 1 (2's complement) for pulse 2.
	change = pulse->t.period + change;
	pulse->target_period = change < 0 ? 0 : change;

	pulse->mute = (pulse->t.period < 8 || pulse->target_period > 0x7ff) ?
		1 : 0;
}

pulse_t pulse_create(uint8_t id)
{
	pulse_t pulse;
	memset(&pulse, 0, sizeof(pulse_t));

	pulse.id          = id;
	pulse.t.step      = 0;
	pulse.t.from      = 0;
	pulse.t.limit     = 7;
	pulse.t.loop      = 1;
	pulse.sweep.limit = 0;
	pulse.enabled     = 0;

	return pulse;
}

void pulse_set_ctrl(pulse_t* pulse, uint8_t value)
{
	pulse->const_volume     = (value & BIT_4) > 0;
	pulse->envelope.loop    = (value & BIT_5) > 0;
	pulse->envelope.period  = value & 0xF;
	pulse->envelope.counter = pulse->envelope.period;
	pulse->envelope.step    = 15;
	pulse->duty             = value >> 6;
}

void pulse_set_timer(pulse_t* pulse, uint8_t value)
{
	pulse->t.period = (pulse->t.period & ~0xff) | value;
	update_target_period(pulse);
}

void pulse_set_sweep(pulse_t* pulse, uint8_t value)
{
	pulse->enable_sweep = (value & BIT_7) > 0;
	pulse->sweep.period = ((value & PULSE_PERIOD) >> 4) + 1;
	pulse->sweep.counter = pulse->sweep.period;
	pulse->shift = value & PULSE_SHIFT;
	pulse->neg = value & BIT_3;
	update_target_period(pulse);
}

void pulse_set_length_counter(pulse_t* pulse, uint8_t value)
{
	pulse->t.period = (pulse->t.period & 0xff) | (value & 0x7) << 8;
	if (pulse->enabled)
		pulse->l = length_counter_lookup[value >> 3];

	update_target_period(pulse);
	pulse->envelope.step = 15;
}

void pulse_length_sweep(pulse_t* pulse)
{
	if (divider_clock(&pulse->sweep) && pulse->enable_sweep &&
	    pulse->shift > 0 && !pulse->mute) {
		pulse->t.period = pulse->target_period;
		update_target_period(pulse);
	}

	// Length counter.
	if (pulse->l && !pulse->envelope.loop)
		pulse->l--;
}
