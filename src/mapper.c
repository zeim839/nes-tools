#include "mapper.h"

#define INES_HEADER_SIZE 16

static void set_mapping
(mapper_t* mapper, uint16_t tl, uint16_t tr, uint16_t bl, uint16_t br)
{
	mapper->nametable_map[0] = tl;
	mapper->nametable_map[1] = tr;
	mapper->nametable_map[2] = bl;
	mapper->nametable_map[3] = br;
}

mapper_t* mapper_from_file(const char* path)
{
	SDL_RWops* file;
	if (!(file = SDL_RWFromFile(path, "rb"))) {
		LOG(ERROR, "file '%s' not found", path);
		return NULL;
	}

	uint8_t header[INES_HEADER_SIZE];
	SDL_RWread(file, header, INES_HEADER_SIZE, 1);
	if (strncmp((const char*)header, "NES\x1A", 4) != 0) {
		LOG(ERROR, "unknown file format");
		exit(EXIT_FAILURE);
	}

	if (header[6] & BIT_2) {
		LOG(ERROR, "Trainer not supported");
		return NULL;
	}

	mapper_t* mapper = malloc(sizeof(mapper_t));
	memset(mapper, 0, sizeof(mapper_t));

	mapper->id = ((header[6] & 0xF0) >> 4) | (header[7] & 0xF0);
	mapper->prg_banks = header[4];
	mapper->chr_banks = header[5];
	mapper->ram_banks = header[8];

	if (mapper->id != 0) {
		LOG(ERROR, "unsupported mapper number #%d", mapper->id);
		free(mapper);
		return NULL;
	}

	if (mapper->ram_banks) {
		mapper->ram_size = 0x2000 * mapper->ram_banks;
		LOG(INFO, "PRG RAM Banks (8kb): %u", mapper->ram_banks);
	}

	mapper->type = (header[9] & 1) ? PAL : NTSC;

	// probably PAL ROM
	if (strstr(path, "(E)") != NULL && mapper->type == NTSC) {
		mapper->type = PAL;
        }

	enum mirroring mirroring = HORIZONTAL;
	if (header[6] & BIT_3) {
		mirroring = FOUR_SCREEN;
	}
	else if (header[6] & BIT_0) {
		mirroring = VERTICAL;
	}

	if (!mapper->ram_banks) {
		LOG(INFO, "PRG RAM Banks (8kb): Not specified, Assuming 8kb");
		mapper->ram_size = 0x2000;
	}

	if (mapper->ram_size) {
		mapper->prg_ram = malloc(mapper->ram_size);
		memset(mapper->prg_ram, 0, mapper->ram_size);
	}

	LOG(INFO, "PRG banks (16KB): %u", mapper->prg_banks);
	LOG(INFO, "CHR banks (8KB): %u", mapper->chr_banks);

	mapper->prg_rom = malloc(0x4000 * mapper->prg_banks);
	SDL_RWread(file, mapper->prg_rom, 0x4000 * mapper->prg_banks, 1);

	if (mapper->chr_banks) {
		mapper->chr_rom = malloc(0x2000 * mapper->chr_banks);
		SDL_RWread(file, mapper->chr_rom, 0x2000 * mapper->chr_banks, 1);
	}
	else {
		LOG(INFO, "Using CHR ROM");
		mapper->chr_ram_size = (mapper->chr_ram_size) ?
			mapper->chr_ram_size : 0x2000;
		mapper->chr_rom = malloc(mapper->chr_ram_size);
		memset(mapper->chr_rom, 0, mapper->chr_ram_size);
	}

	switch (mapper->type) {
        case NTSC:
		LOG(INFO, "ROM type: NTSC");
		break;
        case PAL:
		LOG(INFO, "ROM type: PAL");
		break;
        default:
		LOG(INFO, "ROM type: Unknown");
	}

	LOG(INFO, "Using mapper #%d", mapper->id);
	mapper->clamp = (mapper->prg_banks * 0x4000) - 1;
	SDL_RWclose(file);

	// Set mirroring.
	switch (mirroring) {
        case HORIZONTAL:
		set_mapping(mapper, 0, 0, 0x400, 0x400);
		LOG(DEBUG, "Using mirroring: Horizontal");
		break;
        case VERTICAL:
		set_mapping(mapper,0, 0x400, 0, 0x400);
		LOG(DEBUG, "Using mirroring: Vertical");
		break;
        case FOUR_SCREEN:
		set_mapping(mapper, 0, 0x400, 0x800, 0xC00);
		LOG(DEBUG, "Using mirroring: Four screen");
		break;
        default:
		set_mapping(mapper,0, 0, 0, 0);
		LOG(ERROR, "Unknown mirroring %u", mirroring);
	}

	mapper->mirroring = mirroring;

	return mapper;
}

void mapper_destroy(mapper_t* mapper)
{
	free(mapper->prg_rom);
	free(mapper->chr_rom);
	free(mapper->prg_ram);
	free(mapper);
}

uint8_t mapper_read_rom(mapper_t* mapper, uint8_t bus, uint16_t addr)
{
	if (addr < 0x6000) {
		LOG(DEBUG, "Attempted to read from unavailable expansion ROM");
		return bus;
	}

	if (addr < 0x8000) {
		if (mapper->prg_ram != NULL)
			return mapper->prg_ram[addr - 0x6000];

		LOG(DEBUG, "Attempted to read from non existent PRG RAM");
		return bus;
	}

	return mapper_read_prg(mapper, addr);
}

void mapper_write_rom(mapper_t* mapper, uint16_t addr, uint8_t val)
{
	if (addr < 0x6000){
		LOG(DEBUG, "Attempted to write to unavailable expansion ROM");
		return;
	}

	if (addr < 0x8000){
		if (mapper->prg_ram != NULL) {
			mapper->prg_ram[addr - 0x6000] = val;
			return;
		}
		LOG(DEBUG, "Attempted to write to non existent PRG RAM");
		return;
	}

	mapper_write_prg(mapper, addr, val);
}

uint8_t mapper_read_prg(mapper_t* mapper, uint16_t addr)
{ return mapper->prg_rom[(addr - 0x8000) & mapper->clamp]; }

void mapper_write_prg(mapper_t* mapper, uint16_t addr, uint8_t val)
{ LOG(DEBUG, "Attempted to write to PRG-ROM"); }

uint8_t mapper_read_chr(mapper_t* mapper, uint16_t addr)
{ return mapper->chr_rom[addr]; }

void mapper_write_chr(mapper_t* mapper, uint16_t addr, uint8_t val)
{
	if (!mapper->chr_ram_size){
		LOG(DEBUG, "Attempted to write to CHR-ROM");
		return;
	}
	mapper->chr_rom[addr] = val;
}
