#ifndef NES_TOOLS_GFX_H
#define NES_TOOLS_GFX_H

#include "system.h"

// gfx_t interfaces the SDL window/renderer.
typedef struct
{
	// Window metadata.
	int   width;
	int   height;
	int   screen_width;
	int   screen_height;
	float scale;

	// SDL.
	SDL_Window*       window;
	SDL_Renderer*     renderer;
	SDL_Texture*      texture;
	SDL_AudioDeviceID audio_device;
	TTF_Font*         font;
	SDL_Rect          dest;

} gfx_t;

// gfx_create allocates a new gfx_t, creating a window with the
// specified width, height, and scaling factor.
gfx_t* gfx_create(int width, int height, float scale);
void gfx_destroy(gfx_t* gfx);

// gfx_render writes the texture stored in buffer to the screen.
void gfx_render(gfx_t* gfx, const uint32_t* buffer);

#endif // NES_TOOLS_GFX_H
