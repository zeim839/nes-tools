#ifndef NES_TOOLS_APU_H
#define NES_TOOLS_APU_H

#include "../system.h"
#include "../gfx.h"
#include "../bus.h"

#include "audio.h"
#include "pulse.h"
#include "biquad.h"
#include "triangle.h"
#include "noise.h"
#include "dmc.h"

typedef struct apu_t
{
	bus_t* bus;
	gfx_t* gfx;
	float  volume;

	int16_t buff[AUDIO_BUFF_SIZE];
	size_t  stat_window[STATS_WIN_SIZE];

	pulse_t    pulse1;
	pulse_t    pulse2;
	triangle_t tri;
	noise_t    noise;
	dmc_t      dmc;
	sampler_t  sampler;

	uint8_t frame_mode;
	uint8_t status;
	uint8_t IRQ_inhibit;
	uint8_t frame_interrupt;
	uint8_t audio_start;
	uint8_t reset_sequencer;
	size_t  cycles;
	size_t  sequencer;
	float   stat;
	size_t  stat_index;

	biquad_t filter;
	biquad_t aa_filter;

} apu_t;

apu_t* apu_create(bus_t* bus, gfx_t* gfx);
void apu_destroy(apu_t* apu);
void apu_reset(apu_t* apu);
void apu_exec(apu_t* apu);
void apu_set_status(apu_t* apu, uint8_t val);
float apu_get_sample(apu_t* apu);
void apu_queue_audio(apu_t* apu, gfx_t* gfx);
uint8_t apu_read_status(apu_t* apu);
void apu_set_frame_counter_ctrl(apu_t* apu, uint8_t val);

#endif // NES_TOOLS_APU_H
