#include "snapshot.h"

snapshot_t* snapshot_create(emulator_t* emu)
{
	snapshot_t* snap = malloc(sizeof(snapshot_t));

	snap->cpu    = malloc(sizeof(cpu6502_t));
	snap->ppu    = malloc(sizeof(ppu_t));
	snap->apu    = malloc(sizeof(apu_t));
	snap->bus    = malloc(sizeof(bus_t));
	snap->mapper = malloc(sizeof(mapper_t));

	snapshot_update(snap, emu);

	return snap;
}

void snapshot_update(snapshot_t* snap, emulator_t* emu)
{
	memcpy(snap->cpu, emu->cpu, sizeof(cpu6502_t));
	memcpy(snap->ppu, emu->ppu, sizeof(ppu_t));
	memcpy(snap->apu, emu->apu, sizeof(apu_t));
	memcpy(snap->bus, emu->bus, sizeof(bus_t));
	memcpy(snap->mapper, emu->bus->mapper, sizeof(mapper_t));

	// Update bus addresses.
	snap->cpu->bus = snap->bus;
	snap->ppu->bus = snap->bus;
	snap->apu->bus = snap->bus;

	// Update bus.
	bus_set_cpu(snap->bus, snap->cpu);
	bus_set_ppu(snap->bus, snap->ppu);
	bus_set_apu(snap->bus, snap->apu);

	// Update mapper.
	uint8_t chr_banks = snap->mapper->chr_banks;
	uint8_t prg_banks = snap->mapper->prg_banks;
	uint8_t ram_size = snap->mapper->ram_size;

	snap->chr_rom = malloc(0x2000 * chr_banks);
	snap->prg_rom = malloc(0x4000 * prg_banks);
	snap->prg_ram = malloc(ram_size);

	memcpy(snap->chr_rom, emu->mapper->chr_rom, 0x2000 * chr_banks);
	memcpy(snap->prg_rom, emu->mapper->prg_rom, 0x2000 * prg_banks);
	memcpy(snap->prg_ram, emu->mapper->prg_ram, ram_size);

	// Update joypad, otherwise keys get stuck.
	snap->bus->joy1.status = 0;
	snap->bus->joy2.status = 0;
}

void snapshot_destroy(snapshot_t* snap)
{
	free(snap->cpu);
	free(snap->ppu);
	free(snap->apu);
	free(snap->bus);
	free(snap->mapper);
	free(snap->chr_rom);
	free(snap->prg_rom);
	free(snap->prg_ram);
	free(snap);
}

void snapshot_restore(snapshot_t* snap, emulator_t* emu)
{
	memcpy(emu->cpu, snap->cpu, sizeof(cpu6502_t));
	memcpy(emu->ppu, snap->ppu, sizeof(ppu_t));
	memcpy(emu->apu, snap->apu, sizeof(apu_t));
	memcpy(emu->bus, snap->bus, sizeof(bus_t));
	memcpy(emu->bus->mapper, snap->mapper, sizeof(mapper_t));

	// Update bus addresses.
	emu->cpu->bus = emu->bus;
	emu->ppu->bus = emu->bus;
	emu->apu->bus = emu->bus;

	// Update bus internals.
	bus_set_cpu(emu->bus, emu->cpu);
	bus_set_ppu(emu->bus, emu->ppu);
	bus_set_apu(emu->bus, emu->apu);

	// Update mapper.
	uint8_t chr_banks = snap->mapper->chr_banks;
	uint8_t prg_banks = snap->mapper->prg_banks;
	uint8_t ram_size = snap->mapper->ram_size;

	memcpy(emu->bus->mapper->chr_rom, snap->chr_rom, 0x2000 * chr_banks);
	memcpy(emu->bus->mapper->prg_rom, snap->prg_rom, 0x2000 * prg_banks);
	memcpy(emu->bus->mapper->prg_ram, snap->prg_ram, ram_size);

	SDL_RenderClear(emu->gfx->renderer);
}
