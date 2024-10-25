#include "triangle.h"

const uint8_t triangle_sequence[32] =
{
	15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
};

triangle_t triangle_create()
{
	triangle_t tri;
	memset(&tri, 0, sizeof(triangle_t));

	tri.sequencer.step  = 0;
	tri.sequencer.limit = 31;
	tri.sequencer.from  = 0;
	tri.enabled         = 0;
	tri.halt            = 1;

	return tri;
}

void triangle_set_counter(triangle_t* tri, uint8_t val)
{
	tri->linear_reload = val & 0x7f;
	tri->halt = (val & BIT_7) > 0;
}

void triangle_set_timer_low(triangle_t* tri, uint8_t val)
{ tri->sequencer.period = (tri->sequencer.period & ~0xff) | val; }

void triangle_set_length(triangle_t* tri, uint8_t val)
{
	tri->sequencer.period = (tri->sequencer.period & 0xff) | (val & 0x7) << 8;
	tri->linear_reload_flag = 1;
	if (!tri->enabled)
		return;

	tri->length_counter = length_counter_lookup[val >> 3];
}

uint8_t triangle_clock(triangle_t* tri)
{
	divider_t* divider = &tri->sequencer;
	if (divider->counter) {
		divider->counter--;
		return 0;
	}

	divider->counter = divider->period;
	if (tri->length_counter && tri->linear_counter)
		divider->step++;

	if (divider->limit && divider->step > divider->limit)
		divider->step = divider->from;

	return 1;
}
