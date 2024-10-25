#include "dmc.h"

static uint16_t dmc_rate_index_ntsc[16] =
{
	428, 380, 340, 320, 286, 254, 226, 214,
	190, 160, 142, 128, 106,  84,  72,  54
};

static uint16_t dmc_rate_index_pal[16] =
{
	398, 354, 316, 298, 276, 236, 210, 198,
	176, 148, 132, 118,  98,  78,  66,  50
};

dmc_t dmc_create()
{
	dmc_t dmc;
	memset(&dmc, 0, sizeof(dmc_t));

	dmc.empty   = 1;
	dmc.silence = 1;

	return dmc;
}

void dmc_set_ctrl(dmc_t* dmc, enum tv_system type, uint8_t val)
{
	dmc->loop = (val & BIT_6) > 0;
	dmc->irq_enable = (val & BIT_7) > 0;
	if(!dmc->irq_enable) {
		dmc->interrupt = 0;
	}

	dmc->rate = (type == NTSC) ?
		dmc_rate_index_ntsc[val & 0xf] - 1 :
	        dmc_rate_index_pal[val & 0xf] - 1;
}

void dmc_set_da(dmc_t* dmc, uint8_t val)
{ dmc->counter = val & 0x7F; }

void dmc_set_addr(dmc_t* dmc, uint8_t val)
{ dmc->sample_addr = 0xC000 + (uint16_t)val * 64; }

void dmc_set_length(dmc_t* dmc, uint8_t val)
{ dmc->sample_length = (uint16_t)val * 16 + 1; }
