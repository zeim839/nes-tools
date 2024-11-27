#ifndef NES_TOOLS_SYSTEM_H
#define NES_TOOLS_SYSTEM_H

#include "config.h"

#if defined _WIN32 || defined __CYGWIN__
#define SDL_MAIN_HANDLED
#endif

#include <SDL.h>
#include <SDL_ttf.h>

#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "log.h"

enum
{
	BIT_7 = 1<<7,
	BIT_6 = 1<<6,
	BIT_5 = 1<<5,
	BIT_4 = 1<<4,
	BIT_3 = 1<<3,
	BIT_2 = 1<<2,
	BIT_1 = 1<<1,
	BIT_0 = 1
};

#endif // NES_TOOLS_SYSTEM_H
