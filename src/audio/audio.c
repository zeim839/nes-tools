#include "audio.h"

const uint8_t length_counter_lookup[32] =
{
	// HI/LO 0   1   2   3   4   5   6   7    8   9   A   B   C   D   E   F
	// ----------------------------------------------------------------------
	/* 0 */ 10, 254, 20, 2, 40, 4, 80, 6, 160, 8, 60, 10, 14, 12, 26, 14,
	/* 1 */ 12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

uint8_t divider_clock(divider_t* divider)
{
	if (divider->counter) {
		divider->counter--;
		return 0;
	}

	divider->counter = divider->period;
	divider->step++;

	if (divider->limit && divider->step > divider->limit)
		divider->step = divider->from;

	// Trigger clock.
	return 1;
}

uint8_t divider_clock_inverse(divider_t* divider)
{
	if (divider->counter) {
		divider->counter--;
		return 0;
	}

	divider->counter = divider->period;
	if (divider->limit && divider->step == 0 && divider->loop)
		divider->step = divider->limit;
	else if (divider->step)
		divider->step--;

	// Trigger clock.
	return 1;
}
