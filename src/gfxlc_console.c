#include "gfxlc_console.h"

static void gfxlc_console_destroy_text_cache(gfxlc_console_t *con)
{
    if (con->textTexture)
    {
        SDL_DestroyTexture(con->textTexture);
        con->textTexture = NULL;
    }
    if (con->textSurface)
    {
        SDL_DestroySurface(con->textSurface);
        con->textSurface = NULL;
    }
}

int gfxlc_console_init(gfxlc_console_t **con)
{
    (*con) = (gfxlc_console_t *)calloc(sizeof(gfxlc_console_t), 1);
    if ((*con) == NULL)
    {
        SDL_Log("Unable to allocate memory for gfxlc_console_t.\n");
        return 1;
    }

    gfxlc_console_t *c = (*con);
    c->textSurface = NULL;
    c->textTexture = NULL;
    c->show = false;
    c->overlay_enabled = true;
    c->overlay_color = (SDL_Color){0, 0, 0, 160};
    c->text[0] = '\0';
    const char *base_path = SDL_GetBasePath();
    if (base_path)
    {
        char font_path[1024];

        SDL_snprintf(font_path, sizeof(font_path), "%s/SourceCodePro-Regular.ttf", base_path);

        c->font = TTF_OpenFont(font_path, 10.0f);
        if (c->font == NULL)
        {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return -1;
        }
        SDL_Log("Loading font from: %s\n", font_path);
    }
    return 0;
}

bool gfxlc_console_toggle(gfxlc_console_t *con)
{
    con->show = !con->show;
    return con->show;
}

void gfxlc_console_show(gfxlc_console_t *con)
{
    con->show = true;
}

void gfxlc_console_hide(gfxlc_console_t *con)
{
    con->show = false;
}

bool gfxlc_console_shown(gfxlc_console_t *con)
{
    return con->show;
}

void gfxlc_console_set_overlay(gfxlc_console_t *con, bool enabled, SDL_Color color)
{
    con->overlay_enabled = enabled;
    con->overlay_color = color;
}

void gfxlc_console_add_text(gfxlc_console_t *con, const char *text)
{
    if (text == NULL || text[0] == '\0')
    {
        return;
    }

    if (SDL_strcmp(con->text, text) != 0)
    {
        gfxlc_console_destroy_text_cache(con);
    }

    // currently just replace all the text.
    // only the first 1023 chars of the text will be copied to the console buffer, and it will be null-terminated.
    // TODO append text instead of replace (for history)
    SDL_strlcpy(con->text, text, sizeof(con->text));
}

void gfxlc_console_draw(gfxlc_console_t *con, SDL_Renderer *renderer)
{
    if (!con->show)
    {
        return;
    }

    if (con->overlay_enabled)
    {
        int width = 0;
        int height = 0;
        if (SDL_GetCurrentRenderOutputSize(renderer, &width, &height))
        {
            Uint8 prev_r = 0;
            Uint8 prev_g = 0;
            Uint8 prev_b = 0;
            Uint8 prev_a = 0;
            SDL_BlendMode prev_blend = SDL_BLENDMODE_NONE;

            SDL_GetRenderDrawColor(renderer, &prev_r, &prev_g, &prev_b, &prev_a);
            SDL_GetRenderDrawBlendMode(renderer, &prev_blend);

            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(
                renderer,
                con->overlay_color.r,
                con->overlay_color.g,
                con->overlay_color.b,
                con->overlay_color.a);

            SDL_FRect overlay_rect = {0.0f, 0.0f, (float)width, (float)height};
            SDL_RenderFillRect(renderer, &overlay_rect);

            SDL_SetRenderDrawColor(renderer, prev_r, prev_g, prev_b, prev_a);
            SDL_SetRenderDrawBlendMode(renderer, prev_blend);
        }
    }

    if (con->text[0] == '\0')
    {
        return;
    }

    // create text surface and texture if not already created
    if (!con->textSurface)
    {
        SDL_Color fg = {255, 255, 255, 255};
        con->textSurface = TTF_RenderText_Blended_Wrapped(con->font, con->text, strlen(con->text), fg, 400);
        if (con->textSurface)
        {
            con->textTexture = SDL_CreateTextureFromSurface(renderer, con->textSurface);
            SDL_SetTextureScaleMode(con->textTexture, SDL_SCALEMODE_PIXELART);
        }
    }

    if (con->textTexture)
    {
        SDL_FRect dstRect = {10, 40, con->textSurface->w, con->textSurface->h};
        SDL_RenderTexture(renderer, con->textTexture, NULL, &dstRect);
    }
}

void gfxlc_console_shutdown(gfxlc_console_t *con)
{
    gfxlc_console_destroy_text_cache(con);
}
