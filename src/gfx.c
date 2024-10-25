#include "gfx.h"

// see: font.c
extern unsigned char font_data[31035];

gfx_t* gfx_create(int width, int height, float scale)
{
	if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
		LOG(ERROR, "SDL error: %s", SDL_GetError());
		return NULL;
	}
	if (TTF_Init() == -1) {
		LOG(ERROR, "SDL error: %s", TTF_GetError());
		SDL_Quit();
		return NULL;
	}
	SDL_RWops* rw = SDL_RWFromMem(font_data, sizeof(font_data));
	gfx_t* gfx = malloc(sizeof(gfx_t));
	gfx->width  = width;
	gfx->height = height;
	gfx->scale  = scale;
	if (!(gfx->font = TTF_OpenFontRW(rw, 1, 11))) {
		LOG(ERROR, "SDL error: %s", SDL_GetError());
		SDL_FreeRW(rw);
		free(gfx);
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}
	gfx->window = SDL_CreateWindow(
		"NES Tools",
                SDL_WINDOWPOS_CENTERED,
                SDL_WINDOWPOS_CENTERED,
                gfx->width * (int)gfx->scale,
                gfx->height * (int)gfx->scale,
                SDL_WINDOW_SHOWN
		| SDL_WINDOW_OPENGL
		| SDL_WINDOW_RESIZABLE
		| SDL_WINDOW_ALLOW_HIGHDPI
        );
	if (!gfx->window) {
		LOG(ERROR, SDL_GetError());
		SDL_FreeRW(rw);
		TTF_CloseFont(gfx->font);
		free(gfx);
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}
	SDL_SetWindowMinimumSize(gfx->window, gfx->width, gfx->height);
	gfx->renderer = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED);
	if (!gfx->renderer) {
		LOG(ERROR, SDL_GetError());
		SDL_FreeRW(rw);
		TTF_CloseFont(gfx->font);
		SDL_DestroyWindow(gfx->window);
		free(gfx);
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}
	SDL_RenderSetLogicalSize(gfx->renderer, gfx->width, gfx->height);
	SDL_RenderSetIntegerScale(gfx->renderer, 1);
	SDL_RenderSetScale(gfx->renderer, gfx->scale, gfx->scale);
	gfx->texture = SDL_CreateTexture(
		gfx->renderer,
		SDL_PIXELFORMAT_ABGR8888,
		SDL_TEXTUREACCESS_TARGET,
		gfx->width,
		gfx->height
        );
	if (!gfx->texture) {
		LOG(ERROR, SDL_GetError());
		SDL_FreeRW(rw);
		TTF_CloseFont(gfx->font);
		SDL_DestroyWindow(gfx->window);
		SDL_DestroyRenderer(gfx->renderer);
		free(gfx);
		TTF_Quit();
		SDL_Quit();
		return NULL;
	}
	SDL_SetRenderDrawColor(gfx->renderer, 0, 0, 0, 255);
	SDL_RenderClear(gfx->renderer);
	SDL_RenderPresent(gfx->renderer);
	return gfx;
}

void gfx_destroy(gfx_t* gfx)
{
	TTF_CloseFont(gfx->font);
	TTF_Quit();
	SDL_DestroyTexture(gfx->texture);
	SDL_DestroyRenderer(gfx->renderer);
	SDL_DestroyWindow(gfx->window);
	SDL_CloseAudioDevice(gfx->audio_device);
	SDL_Quit();
	free(gfx);
}

void gfx_render(gfx_t* gfx, const uint32_t* buffer)
{
	SDL_RenderClear(gfx->renderer);
	SDL_UpdateTexture(gfx->texture, NULL, buffer, (int)(gfx->width * sizeof(uint32_t)));
	SDL_RenderCopy(gfx->renderer, gfx->texture, NULL, NULL);
	SDL_SetRenderDrawColor(gfx->renderer, 0, 0, 0, 255);
	SDL_RenderPresent(gfx->renderer);
}
