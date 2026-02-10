#ifndef __GM_FPS_H__
#define __GM_FPS_H__

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
} gm_fps_t;

int gm_fps_init(gm_fps_t **fps);
void gm_fps_draw(gm_fps_t *fps, SDL_Renderer *renderer, TTF_Font *font, int x, int y);
void gm_fps_shutdown(gm_fps_t *fps);

#endif // __GM_FPS_H__