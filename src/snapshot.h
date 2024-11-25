#ifndef NES_TOOLS_SNAPSHOT_H
#define NES_TOOLS_SNAPSHOT_H

#include "system.h"
#include "emulator.h"

// snapshot_t stores the emulator's state at a some point in time.
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

// snapshot_create copies the emulator's state into a new snapshot_t.
snapshot_t* snapshot_create(emulator_t* emu);

// snapshot_update copies an emulator's state into an already
// initialized snapshot_t struct.
void snapshot_update(snapshot_t* snap, emulator_t* emu);

// snapshot_restore writes the snapshot's state into the given emulator.
void snapshot_restore(snapshot_t* snap, emulator_t* emu);

// snapshot_destroy safely deallocates a snapshot_t instance.
void snapshot_destroy(snapshot_t* snap);

#endif // NES_TOOLS_SNAPSHOT_H
