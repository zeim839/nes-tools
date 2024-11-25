#include "apu.h"
#include "../cpu6502.h"

#define TND_LUT_SIZE   203
#define PULSE_LUT_SIZE 31
#define AUDIO_TO_FILE  0

static uint8_t duty[4][8] =
{
	{0, 1, 0, 0, 0, 0, 0, 0}, // 12.5 %
	{0, 1, 1, 0, 0, 0, 0, 0}, // 25 %
	{0, 1, 1, 1, 1, 0, 0, 0}, // 50 %
	{1, 0, 0, 1, 1, 1, 1, 1}  // 25 % negated
};


static float tnd_lut[TND_LUT_SIZE];
static float pulse_lut[PULSE_LUT_SIZE];

static void compute_mixer_lut()
{
	pulse_lut[0] = 0;
	for (int i = 1; i < PULSE_LUT_SIZE; i++)
		pulse_lut[i] = 95.52f / (8128.0f / (float) i + 100);

	tnd_lut[0] = 0;
	for (int i = 1; i < TND_LUT_SIZE; i++)
		tnd_lut[i] = 163.67f / (24329.0f / (float) i + 100);
}

static void sampler_init(apu_t* apu, int frequency)
{
	float cycles_per_frame = apu->bus->mapper->type == PAL? 33247.5: 29780.5;
	float rate = apu->bus->mapper->type == PAL? 50.0f : 60.0f;
	sampler_t* sampler = &apu->sampler;

	// Q = 0.707 => BW = 1.414 (1 octave)
	apu->filter = biquad_create(HPF, 0, 20, frequency, 1);

	// Anti-aliasing filter.
	apu->aa_filter = biquad_create(LPF, 0, 20000, cycles_per_frame * rate, 1);

	sampler->max_period   = cycles_per_frame * rate / frequency;
	sampler->min_period   = sampler->max_period - 1;
	sampler->period       = sampler->min_period;
	sampler->index        = 0;
	sampler->max_index    = AUDIO_BUFF_SIZE;
	sampler->samples      = 0;
	sampler->counter      = 0;
	sampler->factor_index = 0;

	// basically the precision with which we vary the
	// sampling rate. 100 ->2 d.p, 1000->3 d.p, etc.
	sampler->max_factor = 100;

	// This may need to be calibrated to suit the current
	// sampling frequency; the current equilibrium is
	// for 48000 hz.
	sampler->target_factor = sampler->equilibrium_factor = 48;
}

static void init_audio_device(const apu_t* apu)
{
	SDL_AudioSpec want;
	SDL_zero(want);

	// Set the audio format.
	want.freq   = SAMPLING_FREQUENCY;
	want.format = AUDIO_S16SYS;

	want.channels = 1;
	want.callback = NULL;
	want.userdata = NULL;
	want.silence  = 0;

	apu->gfx->audio_device = SDL_OpenAudioDevice(NULL, 0, &want, NULL, 0);
	if (apu->gfx->audio_device == 0) {
		LOG(ERROR , SDL_GetError());
		exit(EXIT_FAILURE);
	}
}

apu_t* apu_create(bus_t* bus, gfx_t* gfx)
{
	compute_mixer_lut();

	apu_t* apu = malloc(sizeof(apu_t));
	memset(apu, 0, sizeof(apu_t));

	apu->volume          = 1;
	apu->bus             = bus;
	apu->gfx             = gfx;
	apu->cycles          = 0;
	apu->sequencer       = 0;
	apu->reset_sequencer = 0;
	apu->audio_start     = 0;
	apu->IRQ_inhibit     = 0;

	// For keeping track of queue_size statistics for use by
	// the adaptive sampler.
	memset(apu->stat_window, 0, sizeof(apu->stat_window));
	apu->stat_index = 0;
	apu->stat       = 0;

	apu->pulse1 = pulse_create(1);
	apu->pulse2 = pulse_create(2);
	apu->tri    = triangle_create();
	apu->noise  = noise_create();
	apu->dmc    = dmc_create();

	sampler_init(apu, SAMPLING_FREQUENCY);
	init_audio_device(apu);
	SDL_PauseAudioDevice(gfx->audio_device, 1);
	apu_set_status(apu, 0);
	apu_set_frame_counter_ctrl(apu, 0);

	return apu;
}

void apu_destroy(apu_t* apu)
{ free(apu); }

void apu_reset(apu_t* apu)
{
	apu_set_status(apu, 0);
	apu->tri.sequencer.step = 0;
	apu->dmc.counter &= 1;
	apu->frame_interrupt = 0;
}

void quarter_frame(apu_t* apu)
{
	triangle_t* tri = &apu->tri;

	divider_clock_inverse(&apu->pulse1.envelope);
	divider_clock_inverse(&apu->pulse2.envelope);
	divider_clock_inverse(&apu->noise.envelope);

	// Triangle linear counter.
	tri->linear_counter = (tri->linear_reload_flag) ?
		tri->linear_reload : tri->linear_counter - 1;

	// If halt is clear, clear linear reload flag.
	tri->linear_reload_flag = tri->halt ? tri->linear_reload_flag : 0;
}

void half_frame(apu_t* apu)
{
	// Length and sweep.
	pulse_length_sweep(&apu->pulse1);
	pulse_length_sweep(&apu->pulse2);

	// Triangle length counter.
	if (!apu->tri.halt && apu->tri.length_counter) {
		apu->tri.length_counter--;
	}

	// Noise length counter.
	noise_t* noise = &apu->noise;
	if (noise->l && !noise->envelope.loop)
		noise->l--;
}

void sample(apu_t* apu)
{
	float sample = biquad_apply(&apu->aa_filter, apu_get_sample(apu));
	sampler_t* sampler = &apu->sampler;
	sampler->counter++;
	if (sampler->counter < sampler->period)
		return;

	apu->buff[sampler->index++] = 32000 * biquad_apply(&apu->filter, sample) * apu->volume;
	if(sampler->index >= sampler->max_index) {
		sampler->index = 0;
	}

	sampler->samples++;
	sampler->counter = 0;

	sampler->period = (apu->sampler.factor_index <= apu->sampler.target_factor) ?
		sampler->max_period : sampler->min_period;

	sampler->factor_index++;
	if(sampler->factor_index > sampler->max_factor)
		sampler->factor_index = 0;
}

void clock_dmc(apu_t* apu)
{
	dmc_t* dmc = &apu->dmc;

	if (dmc->enabled && dmc->empty) {
		if(dmc->bytes_remaining > 0) {
			apu->bus->cpu->dma_cycles += 3;
			dmc->sample = bus_read(apu->bus, dmc->current_addr);
			dmc->empty = 0;
			dmc->bytes_remaining--;
			if(dmc->current_addr == 0xffff)
				dmc->current_addr = 0x8000;
			else
				dmc->current_addr++;
			dmc->irq_set = 0;
		}
		if(dmc->bytes_remaining == 0) {
			if(dmc->loop) {
				dmc->current_addr = dmc->sample_addr;
				dmc->bytes_remaining = dmc->sample_length;
			}else if(dmc->irq_enable && !dmc->irq_set) {
				dmc->interrupt = 1;
				dmc->irq_set = 1;
				cpu_interrupt(apu->bus->cpu, IRQ);
			}
		}
	}

	if(dmc->rate_index > 0) {
		dmc->rate_index--;
		return;
	}
	dmc->rate_index = dmc->rate;

	if(dmc->bits_remaining > 0) {
		// clamped counter update
		if(!dmc->silence) {
			if(dmc->bits & 1) {
				dmc->counter+=2;
				dmc->counter = dmc->counter > 127 ? 127 : dmc->counter;
			}
			else if(dmc->counter > 1)
				dmc->counter-=2;
			dmc->bits >>= 1;
		}
		dmc->bits_remaining--;
	}
	if(dmc->bits_remaining == 0) {
		if(dmc->empty)
			dmc->silence = 1;
		else {
			dmc->bits = dmc->sample;
			dmc->empty = 1;
			dmc->silence = 0;
		}
		dmc->bits_remaining = 8;
	}
}

void apu_exec(apu_t* apu)
{
	// Perform necessary reset after $4017 write
	if (apu->reset_sequencer) {
		apu->reset_sequencer = 0;
		if (apu->frame_mode == 1) {
			quarter_frame(apu);
			half_frame(apu);
		}
		apu->sequencer = 0;
		goto post_sequencer;
	}

	switch (apu->bus->mapper->type) {
	case NTSC:
	default:
		switch (apu->sequencer) {
		case 0:
			apu->sequencer++;
			break;
		case 7457:
			quarter_frame(apu);
			apu->sequencer++;
			break;
		case 14913:
			quarter_frame(apu);
			half_frame(apu);
			apu->sequencer++;
			break;
		case 22371:
			quarter_frame(apu);
			apu->sequencer++;
			break;
		case 29828:
			apu->sequencer++;
			break;
		case 29829:
			if (apu->frame_mode == 1) {
				apu->sequencer++;
				break;
			}

			quarter_frame(apu);
			half_frame(apu);
			if (!apu->IRQ_inhibit) {
				apu->frame_interrupt = 1;
				cpu_interrupt(apu->bus->cpu, IRQ);
			}
			apu->sequencer = 0;
			break;
		case 37281:
			quarter_frame(apu);
			half_frame(apu);
			apu->sequencer = 0;
			break;
		default:
			apu->sequencer++;
		}
		break;
	case PAL:
		switch (apu->sequencer) {
		case 0:
			apu->sequencer++;
			break;
		case 8313:
			quarter_frame(apu);
			apu->sequencer++;
			break;
		case 16627:
			quarter_frame(apu);
			half_frame(apu);
			apu->sequencer++;
			break;
		case 24939:
			quarter_frame(apu);
			apu->sequencer++;
			break;
		case 33252:
			apu->sequencer++;
			break;
		case 33253:
			if (apu->frame_mode == 1) {
				apu->sequencer++;
				break;
			}

			quarter_frame(apu);
			half_frame(apu);

			if (!apu->IRQ_inhibit) {
				apu->frame_interrupt = 1;
				cpu_interrupt(apu->bus->cpu, IRQ);
			}
			apu->sequencer = 0;
			break;
		case 41565:
			quarter_frame(apu);
			half_frame(apu);
			apu->sequencer = 0;
			break;
		default:
			apu->sequencer++;
		}
		break;
	}

post_sequencer:

	if (apu->cycles & 1) {
		// channel sequencer
		divider_clock(&apu->pulse1.t);
		divider_clock(&apu->pulse2.t);

		// noise timer
		if (divider_clock(&apu->noise.timer)) {
			noise_t* noise = &apu->noise;
			uint8_t feedback = (noise->shift & BIT_0) ^
				(((noise->mode ? BIT_6 : BIT_1) & noise->shift) > 0);

			noise->shift >>= 1;
			noise->shift |= feedback ? (1 << 14) : 0;
		}
	}

	// DMC
	clock_dmc(apu);

	// triangle timer
	triangle_clock(&apu->tri);

	// sample
	sample(apu);

	apu->cycles++;
}

void apu_queue_audio(apu_t* apu, gfx_t* gfx)
{
	uint32_t queue_size = SDL_GetQueuedAudioSize(gfx->audio_device);
	apu->stat = apu->stat - apu->stat_window[apu->stat_index] + queue_size;
	apu->stat_window[apu->stat_index++] = queue_size;
	if(apu->stat_index >= STATS_WIN_SIZE)
		apu->stat_index = 0;

	size_t avg = apu->stat / STATS_WIN_SIZE;

	// From here we tweak the sampling rate ever so slightly to
	// prevent underruns and runaway latency by minimising
	// deviation from the nominal queue size with a bit of
	// control engineering
	float delta_f, error = (float)avg - NOMINAL_QUEUE_SIZE;
	sampler_t* s = &apu->sampler;

	delta_f = (error >= 0) ?
		(s->max_factor - s->equilibrium_factor) * error / NOMINAL_QUEUE_SIZE :
		(s->equilibrium_factor * error / NOMINAL_QUEUE_SIZE);

	s->target_factor = s->equilibrium_factor + delta_f;
	if(s->target_factor > s->max_factor)
		s->target_factor = s->max_factor;

	SDL_QueueAudio(gfx->audio_device, apu->buff, s->index * 2);

	// wait till queue is filled to prevent early onset underruns
	if(!apu->audio_start && queue_size >= NOMINAL_QUEUE_SIZE) {
		SDL_PauseAudioDevice(apu->gfx->audio_device, 0);
		apu->audio_start = 1;
	}

	memset(apu->buff, 0, AUDIO_BUFF_SIZE * 2);

	// reset sampler
	s->index = 0;
}


float apu_get_sample(apu_t* apu)
{
	uint8_t pulse_out = 0;
	uint8_t tnd_out = 0;

	if (apu->pulse1.enabled && apu->pulse1.l && !apu->pulse1.mute)
		pulse_out += (apu->pulse1.const_volume ?
			      apu->pulse1.envelope.period :
			      apu->pulse1.envelope.step) *
			(duty[apu->pulse1.duty][apu->pulse1.t.step]);

	if (apu->pulse2.enabled && apu->pulse2.l && !apu->pulse2.mute)
		pulse_out += (apu->pulse2.const_volume ?
			      apu->pulse2.envelope.period :
			      apu->pulse2.envelope.step) *
			(duty[apu->pulse2.duty][apu->pulse2.t.step]);

	if (apu->tri.enabled && apu->tri.sequencer.period > 1)
		tnd_out += triangle_sequence[apu->tri.sequencer.step] * 3;

	if (apu->noise.enabled && !(apu->noise.shift & BIT_0) && apu->noise.l > 0)
		tnd_out += 2 * (apu->noise.const_volume ?
				apu->noise.envelope.period :
				apu->noise.envelope.step);

	tnd_out += apu->dmc.counter;
	float amp = pulse_lut[pulse_out] + tnd_lut[tnd_out];

	// clamp to within 1 just in case
	return amp > 1 ? 1 : amp;
}

void apu_set_status(apu_t* apu, uint8_t value)
{
	apu->pulse1.enabled   = (value & BIT_0) > 0;
	apu->pulse2.enabled   = (value & BIT_1) > 0;
	apu->tri.enabled      = (value & BIT_2) > 0;
	apu->noise.enabled    = (value & BIT_3) > 0;
	apu->dmc.enabled      = (value & BIT_4) > 0;

	if (apu->dmc.enabled && apu->dmc.bytes_remaining == 0) {
		apu->dmc.bytes_remaining = apu->dmc.sample_length;
		apu->dmc.current_addr = apu->dmc.sample_addr;
	}
	else if(!apu->dmc.enabled) {
		apu->dmc.bytes_remaining = 0;
	}

	apu->dmc.interrupt = 0;

	// reset length counters if disabled
	apu->pulse1.l = apu->pulse1.enabled ? apu->pulse1.l : 0;
	apu->pulse2.l = apu->pulse2.enabled ? apu->pulse2.l : 0;
	apu->tri.length_counter = apu->tri.enabled ?
		apu->tri.length_counter : 0;

	apu->noise.l = apu->noise.enabled ? apu->noise.l : 0;
}

uint8_t apu_read_status(apu_t* apu)
{
	uint8_t status = (apu->pulse1.l > 0);
	status |= (apu->pulse2.l > 0 ? BIT_1 : 0);
	status |= (apu->tri.length_counter > 0 ? BIT_2 : 0);
	status |= (apu->noise.l > 0 ? BIT_3 : 0);
	status |= (apu->frame_interrupt ? BIT_6 : 0);
	status |= (apu->dmc.interrupt ? BIT_7 : 0);
	status |= (apu->dmc.bytes_remaining? BIT_4: 0);
	apu->frame_interrupt = 0;
	return status;
}

void apu_set_frame_counter_ctrl(apu_t* apu, uint8_t value)
{
	// $4017
	apu->IRQ_inhibit = (value & BIT_6) > 0;
	apu->frame_mode = (value & BIT_7) > 0;

	// clear interrupt if IRQ disable set
	if (value & BIT_6)
		apu->frame_interrupt = 0;

	apu->reset_sequencer = 1;
}
