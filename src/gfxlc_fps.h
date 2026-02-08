#ifndef __GFXLC_FPS_H__
#define __GFXLC_FPS_H__

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stddef.h>

typedef struct
{
    // fps texture related state
    int frameCount;
    uint64_t lastUpdateTime;
    SDL_Texture *fpsTexture;
    SDL_FRect fpsRect;
    float currentFPS;
} gfxlc_fps_t;

int gfxlc_fps_init(gfxlc_fps_t **fps);
void gfxlc_fps_draw(gfxlc_fps_t *fps, SDL_Renderer *renderer, TTF_Font *font, int x, int y);
void gfxlc_fps_shutdown(gfxlc_fps_t *fps);

#endif // __GFXLC_FPS_H__