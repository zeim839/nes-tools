#include "joypad.h"

joypad_t joypad_create(uint8_t player)
{
	return (joypad_t){
		.strobe = 0,
		.index  = 0,
		.status = 0,
		.player = player,
	};
}

uint8_t joypad_read(joypad_t* joy)
{
	if(joy->index > 7)
		return 1;

	uint8_t val = (joy->status & (1 << joy->index)) != 0;
	if(!joy->strobe)
		joy->index++;

	return val;
}

void joypad_write(joypad_t* joy, uint8_t data)
{
	joy->strobe = data & 1;
	joy->index = (joy->strobe) ? 0 : joy->index;
}

void joypad_trigger_turbo(joypad_t* joy)
{ joy->status ^= joy->status >> 8; }

void joypad_update(joypad_t* joy, SDL_Event* event)
{
	uint16_t key = 0;
	switch (event->key.keysym.sym) {
	case SDLK_RIGHT:
		key = RIGHT;
		break;
        case SDLK_LEFT:
		key = LEFT;
		break;
        case SDLK_DOWN:
		key = DOWN;
		break;
        case SDLK_UP:
		key = UP;
		break;
        case SDLK_RETURN:
		key = START;
		break;
        case SDLK_RSHIFT:
		key = SELECT;
		break;
        case SDLK_j:
		key = BUTTON_A;
            break;
        case SDLK_k:
		key = BUTTON_B;
		break;
        case SDLK_l:
		key = TURBO_B;
		break;
        case SDLK_h:
		key = TURBO_A;
		break;
	}

	if (event->type == SDL_KEYUP) {
		joy->status &= ~key;
		if (key == TURBO_A)
			joy->status &= ~BUTTON_A;

		if (key == TURBO_B)
			joy->status &= ~BUTTON_B;

		return;
	}

	if (event->type == SDL_KEYDOWN) {
		joy->status |= key;
		if (key == TURBO_A)
			joy->status |= BUTTON_A;

		if (key == TURBO_B)
			joy->status |= BUTTON_B;

		return;
	}
}
