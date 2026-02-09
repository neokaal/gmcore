#ifndef __GFXLC_CONSOLE_H__
#define __GFXLC_CONSOLE_H__

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct
{
    SDL_Surface *textSurface;
    SDL_Texture *textTexture;
    TTF_Font *font;
    bool show;
    char text[1024];
} gfxlc_console_t;

int gfxlc_console_init(gfxlc_console_t **con);

bool gfxlc_console_toggle(gfxlc_console_t *con);
void gfxlc_console_show(gfxlc_console_t *con);
void gfxlc_console_hide(gfxlc_console_t *con);
bool gfxlc_console_shown(gfxlc_console_t *con);

void gfxlc_console_add_text(gfxlc_console_t *con, const char *text);

void gfxlc_console_draw(gfxlc_console_t *con, SDL_Renderer *renderer);
void gfxlc_console_shutdown(gfxlc_console_t *con);

#endif // __GFXLC_CONSOLE_H__