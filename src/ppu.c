#include "ppu.h"
#include "cpu6502.h"

const size_t screen_size = sizeof(uint32_t) * VISIBLE_SCANLINES * VISIBLE_DOTS;
uint32_t ppu_palette[64];

static void to_pixel_format
(const uint32_t* in, uint32_t* out, size_t size, uint32_t format)
{
	for(int i = 0; i < size; i++) {
		switch (format) {
		case SDL_PIXELFORMAT_ARGB8888:{
			out[i] = in[i];
			break;
		}
		case SDL_PIXELFORMAT_ABGR8888:{
			out[i] = (in[i] & 0xff000000) |
				((in[i] << 16) & 0x00ff0000) |
				(in[i] & 0x0000ff00) |
				((in[i] >> 16) & 0x000000ff);

			break;
		}
		default:
			LOG(DEBUG, "Unsupported format");
			exit(EXIT_FAILURE);
		}
	}
}

ppu_t* ppu_create(bus_t* bus)
{
	to_pixel_format(ppu_palette_raw, ppu_palette, 64,
                SDL_PIXELFORMAT_ABGR8888);

	ppu_t* ppu = malloc(sizeof(ppu_t));
	ppu->screen = malloc(screen_size);
	ppu->bus = bus;

	ppu->scanlines_per_frame = bus->mapper->type == NTSC ?
		NTSC_SCANLINES_PER_FRAME : PAL_SCANLINES_PER_FRAME;

	memset(ppu->palette, 0, sizeof(ppu->palette));
	memset(ppu->oam_cache, 0, sizeof(ppu->oam_cache));
	memset(ppu->v_ram, 0, sizeof(ppu->v_ram));
	memset(ppu->oam, 0, sizeof(ppu->oam));

	ppu->oam_addr = 0;
	ppu->v = 0;
	ppu_reset(ppu);

	return ppu;
}

void ppu_destroy(ppu_t* ppu)
{
	free(ppu->screen);
	free(ppu);
}

void ppu_reset(ppu_t* ppu)
{
	ppu->t = ppu->x    = ppu->dots = 0;
	ppu->scanlines     = 261;
	ppu->w             = 1;
	ppu->ctrl         &= ~0xFC;
	ppu->mask          = 0;
	ppu->status        = 0;
	ppu->frames        = 0;
	ppu->oam_cache_len = 0;

	memset(ppu->oam_cache, 0, 8);
	memset(ppu->screen, 0, screen_size);
}

uint8_t ppu_read_status(ppu_t* ppu)
{
	uint8_t status = ppu->status;
	ppu->w = 1;
	ppu->status &= ~BIT_7;
	return status;
}

uint8_t ppu_read(ppu_t* ppu)
{
	uint8_t prev_buff = ppu->buffer;
	ppu->buffer = ppu_read_vram(ppu, ppu->v);

	uint8_t data = (ppu->v >= 0x3F00) ?
		ppu->buffer : prev_buff;

	ppu->v += ((ppu->ctrl & BIT_2) ? 32 : 1);
	return data;
}

void ppu_set_ctrl(ppu_t* ppu, uint8_t ctrl)
{
	ppu->ctrl = ctrl;
	ppu->t &= ~0xc00;
	ppu->t |= (ctrl & BASE_NAMETABLE) << 10;
}

void ppu_write(ppu_t* ppu, uint8_t val)
{
	ppu_write_vram(ppu, ppu->v, val);
	ppu->v += ((ppu->ctrl & BIT_2) ? 32 : 1);
}

void ppu_dma(ppu_t* ppu, uint8_t addr)
{
	bus_t* bus = ppu->bus;
	uint8_t* ptr = bus_get_ptr(bus, addr * 0x100);
	if (ptr == NULL) {
		// Probably in PRG ROM so it is not possible to
		// resolve a pointer due to bank switching, so we do
		// it the slow hard way.
		for (int i = 0; i < 256; i++) {
			ppu->oam[(ppu->oam_addr + i) & 0xff] = bus_read(bus, addr * 0x100 + i);
		}

		cpu_dma_suspend(ppu->bus->cpu);
		return;
	}

	// copy from OAM address to the end (256 bytes)
	memcpy(ppu->oam + ppu->oam_addr, ptr, 256 - ppu->oam_addr);

	// Wrap around and copy from start to OAM address
	// if OAM is not 0x00
	if (ppu->oam_addr)
		memcpy(ppu->oam, ptr + (256 - ppu->oam_addr), ppu->oam_addr);

	// Last value.
	bus->bus = ptr[255];

	cpu_dma_suspend(ppu->bus->cpu);
}

void ppu_set_scroll(ppu_t* ppu, uint8_t val)
{
	// First write.
	if (ppu->w) {
		ppu->t &= ~X_SCROLL_BITS;
		ppu->t |= (val >> 3) & X_SCROLL_BITS;
		ppu->x = val & 0x7;
		ppu->w = 0;
		return;
	}

	// Second write.
	ppu->t &= ~Y_SCROLL_BITS;
	ppu->t |= ((val & 0x7) << 12) | ((val & 0xF8) << 2);
	ppu->w = 1;
}

void ppu_set_addr(ppu_t* ppu, uint8_t addr)
{
	// First write.
	if (ppu->w) {
		ppu->t &= 0xff;
		ppu->t |= (addr & 0x3f) << 8;
		ppu->w = 0;
		return;
	}

	// Second write.
	ppu->t &= 0xff00;
	ppu->t |= addr;
	ppu->v = ppu->t;
	ppu->w = 1;
}

void ppu_set_oam_addr(ppu_t* ppu, uint8_t addr)
{ ppu->oam_addr = addr; }

uint8_t ppu_read_oam(ppu_t* ppu)
{ return ppu->oam[ppu->oam_addr]; }

void ppu_write_oam(ppu_t* ppu, uint8_t val)
{ ppu->oam[ppu->oam_addr++] = val; }

uint8_t ppu_read_vram(ppu_t* ppu, uint16_t addr)
{
	addr = addr & 0x3fff;
	ppu->ppu_bus = addr;

	if (addr < 0x2000) {
		ppu->ppu_bus = mapper_read_chr(ppu->bus->mapper, addr);
		return ppu->ppu_bus;
	}

	if (addr < 0x3F00) {
		addr = (addr & 0xefff) - 0x2000;
		ppu->ppu_bus = ppu->v_ram[
			ppu->bus->mapper->nametable_map[addr / 0x400] +
			(addr & 0x3ff)
		];
		return ppu->ppu_bus;
	}

	if (addr < 0x4000)
		return ppu->palette[(addr - 0x3F00) % 0x20];

	return 0;
}

void ppu_write_vram(ppu_t* ppu, uint16_t addr, uint8_t val)
{
	addr = addr & 0x3fff;
	ppu->ppu_bus = val;

	if (addr < 0x2000) {
		mapper_write_chr(ppu->bus->mapper, addr, val);
		return;
	}

	if (addr < 0x3F00) {
		addr = (addr & 0xefff) - 0x2000;
		ppu->v_ram[ppu->bus->mapper->nametable_map[addr / 0x400]
			   + (addr & 0x3ff)] = val;
		return;
	}

	if (addr >= 0x4000)
		return;

	addr = (addr - 0x3F00) % 0x20;
	if (addr % 4 == 0) {
		ppu->palette[addr] = val;
		ppu->palette[addr ^ 0x10] = val;
	}
	else ppu->palette[addr] = val;

	return;
}

static uint16_t render_background(ppu_t* ppu)
{
	int x = (int)ppu->dots - 1;
	uint8_t fine_x = ((uint16_t)ppu->x + x) % 8;

	if (!(ppu->mask & SHOW_BG_8) && x < 8)
		return 0;

	uint16_t tile_addr = 0x2000 | (ppu->v & 0xFFF);
	uint16_t attr_addr = 0x23C0 | (ppu->v & 0x0C00) |
		((ppu->v >> 4) & 0x38) | ((ppu->v >> 2) & 0x07);

	uint16_t pattern_addr = (ppu_read_vram(ppu, tile_addr) * 16 +
                ((ppu->v >> 12) & 0x7)) | ((ppu->ctrl & BG_TABLE) << 8);

	uint16_t palette_addr = (ppu_read_vram(ppu, pattern_addr) >>
                (7 ^ fine_x)) & 1;

	palette_addr |= ((ppu_read_vram(ppu, pattern_addr + 8) >>
                (7 ^ fine_x)) & 1) << 1;

	if (!palette_addr)
		return 0;

	uint8_t attr = ppu_read_vram(ppu, attr_addr);

	return palette_addr |
		(((attr >> (((ppu->v >> 4) & 4) | (ppu->v & 2))) & 0x3) << 2);
}

static uint16_t render_sprites
(ppu_t* restrict ppu, uint16_t bg_addr, uint8_t* restrict back_priority)
{
	// 4 bytes per sprite
	// byte 0 -> y index
	// byte 1 -> tile index
	// byte 2 -> render info
	// byte 3 -> x index
	int x = (int)ppu->dots - 1, y = (int)ppu->scanlines;
	uint16_t palette_addr = 0;
	uint8_t length = ppu->ctrl & LONG_SPRITE ? 16: 8;
	for(int j = 0; j < ppu->oam_cache_len; j++) {
		int i = ppu->oam_cache[j];
		uint8_t tile_x = ppu->oam[i + 3];

		if (x - tile_x < 0 || x - tile_x >= 8)
			continue;

		uint16_t tile = ppu->oam[i + 1];
		uint8_t tile_y = ppu->oam[i] + 1;
		uint8_t attr = ppu->oam[i + 2];
		int x_off = (x - tile_x) % 8, y_off = (y - tile_y) % length;

		if (!(attr & FLIP_HORIZONTAL))
			x_off ^= 7;
		if (attr & FLIP_VERTICAL)
			y_off ^= (length - 1);

		uint16_t tile_addr;

		if (ppu->ctrl & LONG_SPRITE) {
			y_off = (y_off & 7) | ((y_off & 8) << 1);
			tile_addr = (tile >> 1) * 32 + y_off;
			tile_addr |= (tile & 1) << 12;
		} else {
			tile_addr = tile * 16 + y_off +
				(ppu->ctrl & SPRITE_TABLE ? 0x1000 : 0);
		}

		palette_addr = (ppu_read_vram(ppu, tile_addr) >> x_off) & 1;
		palette_addr |= ((ppu_read_vram(ppu, tile_addr + 8) >> x_off) & 1) << 1;

		if (!palette_addr)
			continue;

		palette_addr |= 0x10 | ((attr & 0x3) << 2);
		*back_priority = attr & BIT_5;

		// Sprite hit evaluation.
		if (!(ppu->status & SPRITE_0_HIT)
		    && (ppu->mask & SHOW_BG)
		    && i == 0
		    && palette_addr
		    && bg_addr
		    && x < 255)
			ppu->status |= SPRITE_0_HIT;
		break;
	}

	return palette_addr;
}

void ppu_exec(ppu_t* ppu)
{
	if (ppu->scanlines < VISIBLE_SCANLINES) {
		// render scanlines 0 - 239
		if (ppu->dots > 0 && ppu->dots <= VISIBLE_DOTS) {
			int x = (int)ppu->dots - 1;
			uint8_t fine_x = ((uint16_t)ppu->x + x) % 8, palette_addr = 0, palette_addr_sp = 0, back_priority = 0;

			if (ppu->mask & SHOW_BG) {
				palette_addr = render_background(ppu);
				if (fine_x == 7) {
					if ((ppu->v & COARSE_X) == 31) {
						ppu->v &= ~COARSE_X;
						// switch horizontal nametable
						ppu->v ^= 0x400;
					}
					else
						ppu->v++;
				}
			}
			if (ppu->mask & SHOW_SPRITE && ((ppu->mask & SHOW_SPRITE_8) || x >=8)) {
				palette_addr_sp = render_sprites(ppu, palette_addr, &back_priority);
			}
			if ((!palette_addr && palette_addr_sp) || (palette_addr && palette_addr_sp && !back_priority))
				palette_addr = palette_addr_sp;

			palette_addr = ppu->palette[palette_addr];
			ppu->screen[ppu->scanlines * VISIBLE_DOTS + ppu->dots - 1] = ppu_palette[palette_addr];
		}
		if (ppu->dots == VISIBLE_DOTS + 1 && ppu->mask & SHOW_BG) {
			if ((ppu->v & FINE_Y) != FINE_Y) {
				// increment coarse x
				ppu->v += 0x1000;
			}
			else{
				ppu->v &= ~FINE_Y;
				uint16_t coarse_y = (ppu->v & COARSE_Y) >> 5;
				if (coarse_y == 29) {
					coarse_y = 0;
					// toggle bit 11 to switch vertical nametable
					ppu->v ^= 0x800;
				}
				else if (coarse_y == 31) {
					// nametable not switched
					coarse_y = 0;
				}
				else{
					coarse_y++;
				}

				ppu->v = (ppu->v & ~COARSE_Y) | (coarse_y << 5);
			}
		}
		else if (ppu->dots == VISIBLE_DOTS + 2 && (ppu->mask & RENDER_ENABLED)) {
			ppu->v &= ~HORIZONTAL_BITS;
			ppu->v |= ppu->t & HORIZONTAL_BITS;
		}
		else if (ppu->dots == VISIBLE_DOTS + 4 && ppu->mask & SHOW_SPRITE && ppu->mask & SHOW_BG) {
			//ppu->mapper->on_scanline(ppu->mapper);
		}
		else if (ppu->dots == END_DOT && ppu->mask & RENDER_ENABLED) {
			memset(ppu->oam_cache, 0, 8);
			ppu->oam_cache_len = 0;
			uint8_t range = ppu->ctrl & LONG_SPRITE ? 16: 8;
			for(size_t i = ppu->oam_addr / 4; i < 64; i++) {
				int diff = (int)ppu->scanlines - ppu->oam[i * 4];
				if (diff >= 0 && diff < range) {
					ppu->oam_cache[ppu->oam_cache_len++] = i * 4;
					if (ppu->oam_cache_len >= 8)
						break;
				}
			}
		}
	}
	else if (ppu->scanlines == VISIBLE_SCANLINES) {
		// post render scanline 240/239
	}
	else if (ppu->scanlines < ppu->scanlines_per_frame) {
		// v blanking scanlines 241 - 261/311
		if (ppu->dots == 1 && ppu->scanlines == VISIBLE_SCANLINES + 1) {
			// set v-blank
			ppu->status |= V_BLANK;
			if (ppu->ctrl & GENERATE_NMI && ppu->bus->cpu) {
				// generate NMI
				cpu_interrupt(ppu->bus->cpu, NMI);
			}
		}
	}
	else{
		// pre-render scanline 262/312
		if (ppu->dots == 1) {
			// reset v-blank and sprite zero hit
			ppu->status &= ~(V_BLANK | SPRITE_0_HIT);
		}
		else if (ppu->dots == VISIBLE_DOTS + 2 && (ppu->mask & RENDER_ENABLED)) {
			ppu->v &= ~HORIZONTAL_BITS;
			ppu->v |= ppu->t & HORIZONTAL_BITS;
		}
		else if (ppu->dots == VISIBLE_DOTS + 4 && ppu->mask & SHOW_SPRITE && ppu->mask & SHOW_BG) {
			//ppu->mapper->on_scanline(ppu->mapper);
		}
		else if (ppu->dots > 280 && ppu->dots <= 304 && (ppu->mask & RENDER_ENABLED)) {
			ppu->v &= ~VERTICAL_BITS;
			ppu->v |= ppu->t & VERTICAL_BITS;
		}
		else if (ppu->dots == END_DOT - 1 && ppu->frames & 1 && ppu->mask & RENDER_ENABLED && ppu->bus->mapper->type == NTSC) {
			// skip one cycle on odd frames if rendering is enabled for NTSC
			ppu->dots++;
		}

		if (ppu->dots >= END_DOT) {
			// inform emulator to render contents of ppu on first dot
			ppu->render = 1;
			ppu->frames++;
		}
	}

	// increment dots and scanlines

	if (++ppu->dots >= DOTS_PER_SCANLINE) {
		if (ppu->scanlines++ >= ppu->scanlines_per_frame)
			ppu->scanlines = 0;
		ppu->dots = 0;
	}
}
