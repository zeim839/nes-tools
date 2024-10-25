#ifndef NES_TOOLS_MAPPER_H
#define NES_TOOLS_MAPPER_H

#include "system.h"

enum tv_system
{
	NTSC = 0,
	PAL
};

enum mirroring
{
	NO_MIRRORING,
	VERTICAL,
	HORIZONTAL,
	FOUR_SCREEN
};

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

mapper_t* mapper_from_file(const char* path);
void mapper_destroy(mapper_t* mapper);

uint8_t mapper_read_rom(mapper_t* mapper, uint8_t bus, uint16_t addr);
void mapper_write_rom(mapper_t* mapper, uint16_t addr, uint8_t val);

uint8_t mapper_read_prg(mapper_t* mapper, uint16_t addr);
void mapper_write_prg(mapper_t* mapper, uint16_t addr, uint8_t val);

uint8_t mapper_read_chr(mapper_t* mapper, uint16_t addr);
void mapper_write_chr(mapper_t* mapper, uint16_t addr, uint8_t val);

#endif // NES_TOOLS_MAPPER_H
