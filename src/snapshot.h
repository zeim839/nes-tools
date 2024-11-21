#ifndef NES_TOOLS_SNAPSHOT_H
#define NES_TOOLS_SNAPSHOT_H

#include "system.h"
#include "emulator.h"

typedef struct
{
	cpu6502_t* cpu;
	ppu_t*     ppu;
	apu_t*     apu;
	bus_t*     bus;

	mapper_t*  mapper;

	uint8_t* chr_rom;
	uint8_t* prg_rom;
	uint8_t* prg_ram;

} snapshot_t;

snapshot_t* snapshot_create(emulator_t* emu);
void snapshot_update(snapshot_t* snap, emulator_t* emu);
void snapshot_restore(snapshot_t* snap, emulator_t* emu);
void snapshot_destroy(snapshot_t* snap);

#endif // NES_TOOLS_SNAPSHOT_H
