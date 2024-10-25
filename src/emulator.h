#ifndef NES_TOOLS_EMULATOR_H
#define NES_TOOLS_EMULATOR_H

#include "system.h"
#include "cpu6502.h"
#include "ppu.h"
#include "bus.h"
#include "audio/apu.h"
#include "mapper.h"
#include "gfx.h"
#include "timerx.h"

// Frame rate in Hz
#define NTSC_FRAME_RATE 60
#define PAL_FRAME_RATE  50

// Turbo keys toggle rate (Hz): value should be a factor of FRAME_RATE
// and should never exceed FRAME_RATE for best result
#define NTSC_TURBO_RATE 30
#define PAL_TURBO_RATE  25

// Sleep time when emulator is paused in milliseconds.
#define IDLE_SLEEP 50

typedef struct
{
	cpu6502_t* cpu;
	ppu_t*     ppu;
	apu_t*     apu;
	bus_t*     bus;

	mapper_t* mapper;
	gfx_t*    gfx;
	double    time_diff;
	uint8_t   exit;
	uint8_t   pause;
	timerx_t  timer;
	uint64_t  period;
	uint64_t  turbo_skip;

	enum tv_system type;

} emulator_t;

emulator_t* emulator_create(mapper_t* mapper);
void emulator_reset(emulator_t* emu);
void emulator_exec(emulator_t* emu);
void emulator_destroy(emulator_t* emu);

#endif // NES_TOOLS_EMULATOR_H
