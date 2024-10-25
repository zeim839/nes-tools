#include "noise.h"

static const uint16_t noise_period_lookup_ntsc[16] =
{
	4, 8, 16, 32, 64, 96, 128, 160, 202,
	254, 380, 508, 762, 1016, 2034, 4068
};

static const uint16_t noise_period_lookup_pal[16] =
{
	4, 8, 14, 30, 60, 88, 118, 148, 188,
	236, 354, 472, 708, 944, 1890, 3778
};

noise_t noise_create()
{
	noise_t noise;
	memset(&noise, 0, sizeof(noise_t));

	noise.enabled     = 0;
	noise.timer.limit = 0;
	noise.shift       = 1;

	return noise;
}

void noise_set_ctrl(noise_t* noise, uint8_t val)
{
	noise->const_volume    = (val & BIT_4) > 0;
	noise->envelope.loop   = (val & BIT_5) > 0;
	noise->envelope.period = val & 0xF;
}

void noise_set_period(noise_t* noise, enum tv_system type, uint8_t val)
{
	noise->timer.period = (type == PAL) ?
		noise_period_lookup_pal[val & 0xF] :
		noise_period_lookup_ntsc[val & 0xF];

	noise->mode = (val & BIT_7) > 0;
}

void noise_set_length(noise_t* noise, uint8_t val)
{
	if (noise->enabled)
		noise->l = length_counter_lookup[val >> 3];

	noise->envelope.step = 15;
}
