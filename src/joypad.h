#ifndef NES_TOOLS_JOYPAD_H
#define NES_TOOLS_JOYPAD_H

#include "system.h"

// Each button's status is enumerated as a single bit in the joypad's
// status attribute.
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

// joypad_t maintains the state of an NES joypad.
typedef struct
{
	uint8_t  strobe;
	uint8_t  index;
	uint16_t status;
	uint8_t  player;

} joypad_t;

// joypad_create initializes a new joypad_t, where player specifies
// the joypad number (two joypads may be used).
joypad_t joypad_create(uint8_t player);

// joypad_read reads from the joypad as though it were mapped to
// main memory (address 0x4016 - 0x4017).
uint8_t joypad_read(joypad_t* joy);

// joypad_write writes to the joypad as though it were mapped to
// main memory (address 0x4016).
void joypad_write(joypad_t* joy, uint8_t data);

// joypad_update updates the status of the joypad according to an SDL
// keyboard event.
void joypad_update(joypad_t* joy, SDL_Event* event);

// joypad_trigger_turbo triggers the turbo button.
void joypad_trigger_turbo(joypad_t* joy);

#endif // NES_TOOLS_JOYPAD_H
