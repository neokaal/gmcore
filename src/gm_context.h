#ifndef __GM_CONTEXT_H__
#define __GM_CONTEXT_H__

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <lua.h>

typedef struct
{
    // SDL window, renderer, and texture
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    // SDL font
    TTF_Font *font;

    // game loop
    int quit;
    SDL_Event evt;
    uint64_t start_ticks;

    // canvas pixel buffer
    uint32_t *pixels;

    // canvas dimensions
    int cvs_width;
    int cvs_height;
    SDL_FRect cvs_on_win_rect;

    // window dimensions
    int win_width;
    int win_height;

} gm_t;

#endif //__GM_CONTEXT_H__