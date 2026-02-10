#ifndef __GM_CONSOLE_H__
#define __GM_CONSOLE_H__

#include <stdbool.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

typedef struct
{
    SDL_Surface *textSurface;
    SDL_Texture *textTexture;
    TTF_Font *font;
    bool show;
    bool overlay_enabled;
    SDL_Color overlay_color;
    char text[1024];
} gm_console_t;

int gm_console_init(gm_console_t **con);

bool gm_console_toggle(gm_console_t *con);
void gm_console_show(gm_console_t *con);
void gm_console_hide(gm_console_t *con);
bool gm_console_shown(gm_console_t *con);
void gm_console_set_overlay(gm_console_t *con, bool enabled, SDL_Color color);

void gm_console_add_text(gm_console_t *con, const char *text);

void gm_console_draw(gm_console_t *con, SDL_Renderer *renderer);
void gm_console_shutdown(gm_console_t *con);

#endif // __GM_CONSOLE_H__
