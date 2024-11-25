#ifndef NES_TOOLS_BUS_H
#define NES_TOOLS_BUS_H

#include "system.h"
#include "mapper.h"
#include "joypad.h"

#define IRQ_ADDRESS         0xFFFE
#define NMI_ADDRESS         0xFFFA
#define RESET_ADDRESS       0xFFFC
#define RAM_SIZE            0x800
#define RAM_END             0x2000
#define IO_REG_MIRRORED_END 0x4000
#define IO_REG_END          0x4020

enum
{
	PPU_CTRL = 0x2000,
	PPU_MASK,
	PPU_STATUS,
	OAM_ADDR,
	OAM_DATA,
	PPU_SCROLL,
	PPU_ADDR,
	PPU_DATA,

	APU_P1_CTRL = 0x4000,
	APU_P1_RAMP,
	APU_P1_FT,
	APU_P1_CT,

	APU_P2_CTRL,
	APU_P2_RAMP,
	APU_P2_FT,
	APU_P2_CT,

	APU_TRI_LINEAR_COUNTER,
	APU_TRI_FREQ1 = 0x400A,
	APU_TRI_FREQ2,

	APU_NOISE_CTRL,
	APU_NOISE_FREQ1 = 0x400E,
	APU_NOISE_FREQ2,

	APU_DMC_CTRL,
	APU_DMC_DA,
	APU_DMC_ADDR,
	APU_DMC_LEN,

	OAM_DMA,

	APU_CTRL,
	APU_STATUS = 0x4015,
	JOY1,
	JOY2,
	FRAME_COUNTER = 0x4017
};

struct cpu6502_t;
struct apu_t;
struct ppu_t;

// bus_t emulates an NES bus.
typedef struct
{
	// Memory.
	mapper_t* mapper;
	uint8_t   ram[RAM_SIZE];
	uint8_t   bus;

	// Joypad controllers.
	joypad_t joy1;
	joypad_t joy2;

	// Bus-connected modules.
	struct cpu6502_t* cpu;
	struct apu_t*     apu;
	struct ppu_t*     ppu;

} bus_t;

bus_t* bus_create(mapper_t* mapper);
void bus_destroy(bus_t* bus);

// bus_write writes val to addr in main memory.
void bus_write(bus_t* bus, uint16_t addr, uint8_t val);

// bus_read fetches the value at addr from main memory.
uint8_t bus_read(bus_t* bus, uint16_t addr);

// bus_get_ptr gets a pointer to the value at addr in main memory.
uint8_t* bus_get_ptr(bus_t* bus, uint16_t addr);

// Eventually set all other NES circuits. bus_read/bus_write will be
// able to access memory-mapped registers.
void bus_set_cpu(bus_t* bus, struct cpu6502_t* cpu);
void bus_set_apu(bus_t* bus, struct apu_t* apu);
void bus_set_ppu(bus_t* bus, struct ppu_t* ppu);

#endif // NES_TOOLS_BUS_H
