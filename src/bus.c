#include "bus.h"
#include "ppu.h"

#include "audio/apu.h"
#include "audio/triangle.h"
#include "audio/noise.h"

#define REQUIRE_PPU(ppu, then) if (ppu == NULL) then;
#define REQUIRE_APU(apu, then) if (apu == NULL) then;

void bus_set_cpu(bus_t* bus, struct cpu6502_t* cpu)
{ bus->cpu = cpu; }

void bus_set_apu(bus_t* bus, struct apu_t* apu)
{ bus->apu = apu; }

void bus_set_ppu(bus_t* bus, struct ppu_t* ppu)
{ bus->ppu = ppu; }

bus_t* bus_create(mapper_t* mapper)
{
	bus_t* bus = malloc(sizeof(bus_t));
	bus->mapper = mapper;

	memset(bus->ram, 0, RAM_SIZE);
	bus->joy1 = joypad_create(0);
	bus->joy2 = joypad_create(1);

	return bus;
}

void bus_destroy(bus_t* bus)
{ free(bus); }

void bus_write(bus_t* bus, uint16_t addr, uint8_t val)
{
	uint8_t old = bus->bus;
        bus->bus = val;
	if (addr < RAM_END) {
		bus->ram[addr % RAM_SIZE] = val;
		return;
	}

	// resolve mirrored registers
	if (addr < IO_REG_MIRRORED_END)
		addr = 0x2000 + (addr - 0x2000) % 0x8;

	// handle all IO registers
	if (addr < IO_REG_END){
		ppu_t* ppu = bus->ppu;
		apu_t* apu = bus->apu;
		switch (addr) {
		case PPU_CTRL:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu_set_ctrl(ppu, val);
			break;
		case PPU_MASK:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu->mask = val;
			break;
		case PPU_SCROLL:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu_set_scroll(ppu, val);
			break;
		case PPU_ADDR:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu_set_addr(ppu, val);
			break;
		case PPU_DATA:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
		        ppu_write(ppu, val);
			break;
		case OAM_ADDR:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu_set_oam_addr(ppu, val);
			break;
		case OAM_DMA:
			REQUIRE_PPU(ppu, break);
			ppu_dma(ppu, val);
			break;
		case OAM_DATA:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			ppu_write_oam(ppu, val);
			break;
		case PPU_STATUS:
			REQUIRE_PPU(ppu, break);
			ppu->ppu_bus = val;
			break;
		case JOY1:
			joypad_write(&bus->joy1, val);
			joypad_write(&bus->joy2, val);
			bus->bus = (old & 0xf0) | (val & 0xf);
			break;
		case APU_P1_CTRL:
			REQUIRE_APU(apu, break);
			pulse_set_ctrl(&apu->pulse1, val);
			break;
		case APU_P2_CTRL:
			REQUIRE_APU(apu, break);
			pulse_set_ctrl(&apu->pulse2, val);
			break;
		case APU_P1_RAMP:
			REQUIRE_APU(apu, break);
			pulse_set_sweep(&apu->pulse1, val);
			break;
		case APU_P2_RAMP:
			REQUIRE_APU(apu, break);
			pulse_set_sweep(&apu->pulse2, val);
			break;
		case APU_P1_FT:
			REQUIRE_APU(apu, break);
			pulse_set_timer(&apu->pulse1, val);
			break;
		case APU_P2_FT:
			REQUIRE_APU(apu, break);
			pulse_set_timer(&apu->pulse2, val);
			break;
		case APU_P1_CT:
			REQUIRE_APU(apu, break);
		        pulse_set_length_counter(&apu->pulse1, val);
			break;
		case APU_P2_CT:
			REQUIRE_APU(apu, break);
			pulse_set_length_counter(&apu->pulse2, val);
			break;
		case APU_TRI_LINEAR_COUNTER:
			REQUIRE_APU(apu, break);
			triangle_set_counter(&apu->tri, val);
			break;
		case APU_TRI_FREQ1:
			REQUIRE_APU(apu, break);
			triangle_set_timer_low(&apu->tri, val);
			break;
		case APU_TRI_FREQ2:
			REQUIRE_APU(apu, break);
			triangle_set_length(&apu->tri, val);
			break;
		case APU_NOISE_CTRL:
			REQUIRE_APU(apu, break);
			noise_set_ctrl(&apu->noise, val);
			break;
		case APU_NOISE_FREQ1:
			REQUIRE_APU(apu, break);
			noise_set_period(&apu->noise, apu->bus->mapper->type, val);
			break;
		case APU_NOISE_FREQ2:
			REQUIRE_APU(apu, break);
			noise_set_length(&apu->noise, val);
			break;
		case APU_DMC_ADDR:
			REQUIRE_APU(apu, break);
			dmc_set_addr(&apu->dmc, val);
			break;
		case APU_DMC_CTRL:
			REQUIRE_APU(apu, break);
			dmc_set_ctrl(&apu->dmc, bus->mapper->type, val);
			break;
		case APU_DMC_DA:
			REQUIRE_APU(apu, break);
			dmc_set_da(&apu->dmc, val);
			break;
		case APU_DMC_LEN:
			REQUIRE_APU(apu, break);
			dmc_set_length(&apu->dmc, val);
			break;
		case FRAME_COUNTER:
			REQUIRE_APU(apu, break);
			apu_set_frame_counter_ctrl(apu, val);
			break;
		case APU_STATUS:
			REQUIRE_APU(apu, break);
			apu_set_status(apu, val);
			break;
		default:
			break;
		}
		return;
	}
	mapper_write_rom(bus->mapper, addr, val);
}

uint8_t bus_read(bus_t* bus, uint16_t addr)
{
	if (addr < RAM_END) {
		bus->bus = bus->ram[addr % RAM_SIZE];
		return bus->bus;
	}

	// resolve mirrored registers
	if (addr < IO_REG_MIRRORED_END)
		addr = 0x2000 + (addr - 0x2000) % 0x8;

	// handle all IO registers
	if (addr < IO_REG_END) {
		ppu_t* ppu = bus->ppu;
		switch (addr) {
		case PPU_STATUS:
			REQUIRE_PPU(ppu, return bus->bus;);
			ppu->ppu_bus &= 0x1f;
			ppu->ppu_bus |= ppu_read_status(ppu) & 0xe0;
			bus->bus = ppu->ppu_bus;
			return bus->bus;
		case OAM_DATA:
			REQUIRE_PPU(ppu, return bus->bus;);
			ppu->ppu_bus = ppu_read_oam(ppu);
			bus->bus = ppu->ppu_bus;
			return bus->bus;
		case PPU_DATA:
			REQUIRE_PPU(ppu, return bus->bus;);
			ppu->ppu_bus = ppu_read(ppu);
			bus->bus = ppu->ppu_bus;
			return bus->bus;
		case PPU_CTRL:
		case PPU_MASK:
		case PPU_SCROLL:
		case PPU_ADDR:
		case OAM_ADDR:
			REQUIRE_PPU(ppu, return bus->bus;);
			bus->bus = ppu->ppu_bus;
			return bus->bus;
		case JOY1:
			bus->bus &= 0xe0;
		        bus->bus |= joypad_read(&bus->joy1) & 0x1f;
			return bus->bus;
		case JOY2:
		        bus->bus &= 0xe0;
			bus->bus |= joypad_read(&bus->joy2) & 0x1f;
			return bus->bus;
		case APU_STATUS:
			REQUIRE_APU(bus->apu, return bus->bus;);
			bus->bus = apu_read_status(bus->apu);
			return bus->bus;
		default: // open bus.
			return bus->bus;
		}
	}

	bus->bus = mapper_read_rom(bus->mapper, bus->bus, addr);
	return bus->bus;
}

uint8_t* bus_get_ptr(bus_t* bus, uint16_t addr)
{
	if (addr < 0x2000)
		return bus->ram + (addr % 0x800);

	if (addr > 0x6000 && addr < 0x8000 && bus->mapper->prg_ram != NULL)
		return bus->mapper->prg_ram + (addr - 0x6000);

	return NULL;
}
