#ifndef NES_TOOLS_JOYPAD_H
#define NES_TOOLS_JOYPAD_H

#include "system.h"

enum
{
	TURBO_B  = 1 << 9,
	TURBO_A  = 1 << 8,
	RIGHT    = 1 << 7,
	LEFT     = 1 << 6,
	DOWN     = 1 << 5,
	UP       = 1 << 4,
	START    = 1 << 3,
	SELECT   = 1 << 2,
	BUTTON_B = 1 << 1,
	BUTTON_A = 1
};

typedef struct
{
	uint8_t  strobe;
	uint8_t  index;
	uint16_t status;
	uint8_t  player;

} joypad_t;

joypad_t joypad_create(uint8_t player);
uint8_t joypad_read(joypad_t* joy);
void joypad_write(joypad_t* joy, uint8_t data);
void joypad_update(joypad_t* joy, SDL_Event* event);
void joypad_trigger_turbo(joypad_t* joy);

#endif // NES_TOOLS_JOYPAD_H
