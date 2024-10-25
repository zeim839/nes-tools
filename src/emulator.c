#include "emulator.h"

emulator_t* emulator_create(mapper_t* mapper)
{
	emulator_t* emu = malloc(sizeof(emulator_t));
	emu->mapper = mapper;
	emu->type   = mapper->type;

	emu->period = (emu->type == PAL) ?
		1000000000 / PAL_FRAME_RATE :
		1000000000 / NTSC_FRAME_RATE;

	emu->turbo_skip = (emu->type == PAL) ?
		PAL_FRAME_RATE / PAL_TURBO_RATE :
		NTSC_FRAME_RATE / NTSC_TURBO_RATE;

	if (!(emu->gfx = gfx_create(256, 240, 2))) {
		free(emu);
		return NULL;
	}

	emu->gfx->screen_width = -1;
	emu->gfx->screen_height = -1;

	if (!(emu->bus = bus_create(emu->mapper))) {
		gfx_destroy(emu->gfx);
		free(emu);
		return NULL;
	}

	if (!(emu->ppu = ppu_create(emu->bus))) {
		bus_destroy(emu->bus);
		gfx_destroy(emu->gfx);
		free(emu);
		return NULL;
	}

	if (!(emu->cpu = cpu_create(emu->bus))) {
		ppu_destroy(emu->ppu);
		bus_destroy(emu->bus);
		gfx_destroy(emu->gfx);
		free(emu);
		return NULL;
	}

	if (!(emu->apu = apu_create(emu->bus, emu->gfx))) {
		cpu_destroy(emu->cpu);
		ppu_destroy(emu->ppu);
		bus_destroy(emu->bus);
		gfx_destroy(emu->gfx);
		free(emu);
		return NULL;
	}

	bus_set_cpu(emu->bus, emu->cpu);
	bus_set_ppu(emu->bus, emu->ppu);
	bus_set_apu(emu->bus, emu->apu);

	emu->timer = timerx_create(emu->period);
	emu->exit  = 0;
	emu->pause = 0;

	return emu;
}

void emulator_exec(emulator_t* emu)
{
	joypad_t* joy1      = &emu->bus->joy1;
	joypad_t* joy2      = &emu->bus->joy2;
	ppu_t* ppu          = emu->ppu;
	cpu6502_t* cpu      = emu->cpu;
	apu_t* apu          = emu->apu;
	gfx_t* gfx          = emu->gfx;
	timerx_t* timer      = &emu->timer;
	timerx_t frame_timer = timerx_create(emu->period);

	SDL_Event e;
	timerx_mark_start(&frame_timer);

	while (!emu->exit) {
		timerx_mark_start(timer);
		while (SDL_PollEvent(&e)) {
			joypad_update(joy1, &e);
			joypad_update(joy2, &e);
			if ((joy1->status & 0xc) == 0xc ||
			    (joy2->status & 0xc) == 0xc) {
				emulator_reset(emu);
			}

			switch (e.type) {
			case SDL_KEYDOWN:
				switch (e.key.keysym.sym) {
				case SDLK_ESCAPE:
					emu->exit = 1;
					break;
				case SDLK_AUDIOPLAY:
				case SDLK_SPACE:
					emu->pause ^= 1;
					break;
				case SDLK_F5:
				        emulator_reset(emu);
					break;
				default:
					break;
				}
				break;
			case SDL_QUIT:
				emu->exit = 1;
				break;
			default:
				if(e.key.keysym.sym == SDLK_AC_BACK
				   || e.key.keysym.scancode == SDL_SCANCODE_AC_BACK) {
					emu->exit = 1;
					LOG(DEBUG, "Exiting emulator session");
				}
			}
		}

		// Trigger turbo events
		if (ppu->frames % emu->turbo_skip == 0) {
			joypad_trigger_turbo(joy1);
			joypad_trigger_turbo(joy2);
		}

		if (!emu->pause) {
			// If ppu.render is set a frame is complete
			if (emu->type == NTSC) {
				while (!ppu->render) {
					ppu_exec(ppu);
					ppu_exec(ppu);
					ppu_exec(ppu);
					cpu_exec(cpu);
					apu_exec(apu);
				}
			}
			else {

				// PAL
				uint8_t check = 0;
				while (!ppu->render) {
					ppu_exec(ppu);
					ppu_exec(ppu);
					ppu_exec(ppu);
					check++;
					if(check == 5) {
						// on the fifth run execute an extra ppu clock
						// this produces 3.2 scanlines per cpu clock
						ppu_exec(ppu);
						check = 0;
					}
					cpu_exec(cpu);
					apu_exec(apu);
				}
			}
			gfx_render(gfx, ppu->screen);
			ppu->render = 0;
			apu_queue_audio(apu, gfx);
			timerx_mark_end(timer);
			timerx_adjusted_wait(timer);

		} else {
			timerx_wait(IDLE_SLEEP);
		}
	}
	timerx_mark_end(&frame_timer);
	emu->time_diff = timerx_get_diff(&frame_timer);
}

void emulator_reset(emulator_t* emu)
{
	LOG(INFO, "Resetting emulator");
	cpu_reset(emu->cpu);
	apu_reset(emu->apu);
	ppu_reset(emu->ppu);
}

void emulator_destroy(emulator_t* emu)
{
	LOG(DEBUG, "Starting emulator clean up");

	apu_destroy(emu->apu);
	ppu_destroy(emu->ppu);
	cpu_destroy(emu->cpu);
	gfx_destroy(emu->gfx);
	bus_destroy(emu->bus);
	free(emu);

	LOG(DEBUG, "Emulator session successfully terminated");
}
