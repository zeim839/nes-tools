#ifndef NES_TOOLS_DMC_H
#define NES_TOOLS_DMC_H

#include "../system.h"
#include "../mapper.h"
#include "audio.h"

typedef struct
{
	uint8_t  enabled;
	uint8_t  irq_enable;
	uint8_t  loop;
	uint8_t  counter;
	uint16_t sample_length;
	uint16_t sample_addr;
	uint8_t  interrupt;
	uint8_t  irq_set;
	uint16_t rate;
	uint16_t rate_index;

	// Output unit
	uint8_t bits_remaining;
	uint8_t silence;
	uint8_t bits;

	// Memory reader.
	uint8_t  sample;
	uint8_t  empty;
	uint16_t bytes_remaining;
	uint16_t current_addr;

} dmc_t;

dmc_t dmc_create();
void dmc_set_ctrl(dmc_t* dmc, enum tv_system type, uint8_t val);
void dmc_set_da(dmc_t* dmc, uint8_t val);
void dmc_set_addr(dmc_t* dmc, uint8_t val);
void dmc_set_length(dmc_t* dmc, uint8_t val);

#endif // NES_TOOLS_DMC_H
