#ifndef NES_TOOLS_MAPPER_H
#define NES_TOOLS_MAPPER_H

#include "system.h"

// The NES console had two main variants based on different TV display
// standards: NTSC (used primarily in Japan/USA) and PAL (used in
// Europe, Australia, etc.). The variant is specified in iNES file
// format.
enum tv_system
{
	NTSC = 0,
	PAL
};

// Nametable mirroring configuration. See:
// https://www.nesdev.org/wiki/Mirroring#Nametable_Mirroring
enum mirroring
{
	NO_MIRRORING,
	VERTICAL,
	HORIZONTAL,
	FOUR_SCREEN
};

// mapper_t stores data for an iNES mapper/cartridge.
typedef struct
{
	uint8_t* chr_rom;
	uint8_t* prg_rom;
	uint8_t* prg_ram;

	uint16_t prg_banks;
	uint16_t chr_banks;
	uint16_t ram_banks;

	enum mirroring mirroring;
	enum tv_system type;
	size_t ram_size;
	size_t chr_ram_size;

	uint16_t nametable_map[4];

	uint32_t clamp;
	uint8_t  id;

} mapper_t;

// mapper_from_file creates a mapper_t instance from a '.nes' file.
mapper_t* mapper_from_file(const char* path);
void mapper_destroy(mapper_t* mapper);

// mapper_read_rom fetches data from CPU-addressable locations in the
// mapper circuit's memory. If an address is invalid, it returns bus.
uint8_t mapper_read_rom(mapper_t* mapper, uint8_t bus, uint16_t addr);

// mapper_write_rom writes to a CPU-addressable location in the mapper
// circuit's memory.
void mapper_write_rom(mapper_t* mapper, uint16_t addr, uint8_t val);

// mapper_read_prg decodes a CPU-addressable address to a value in the
// mapper's PRG ROM memory.
uint8_t mapper_read_prg(mapper_t* mapper, uint16_t addr);

// mapper_read_prg decodes a CPU-addressable address and writes a
// value in the mapper's PRG ROM memory.
void mapper_write_prg(mapper_t* mapper, uint16_t addr, uint8_t val);

// mapper_read_chr fetches the value at addr from the mapper's CHR ROM.
uint8_t mapper_read_chr(mapper_t* mapper, uint16_t addr);

// mapper_read_chr writes val to the mapper's CHR ROM at the given addr.
void mapper_write_chr(mapper_t* mapper, uint16_t addr, uint8_t val);

#endif // NES_TOOLS_MAPPER_H
