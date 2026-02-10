// Copyright (c) 2026 Neokaal Tech Private Limited
// Licensed under the MIT license. See LICENSE file in the project root for details.

/**
 * @file main.c
 * @author Abhishek Mishra
 * @brief Main program for gmcore
 * @version 0.1
 * @date 2026-02-10
 *
 * @copyright Copyright (c) 2026 Neokaal Tech Private Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "gm_util.h"
#include "gm_context.h"
#include "gm_lua.h"
#include "gm_fps.h"
#include "gm_console.h"

#define CNV_W 320
#define CNV_H 240

#define GFX_W (CNV_W * 2)
#define GFX_H (CNV_H * 2)

int gm_init(gm_t *gfxlc);
int gm_draw(gm_t *gfxlc, gm_lua_t *lua_ctx, gm_fps_t *fps, gm_console_t *console);
void gm_shutdown(gm_t *gfxlc);
int load_fonts(gm_t *gfxlc);

int main(int argc, char *argv[])
{
    gm_t *gfxctx = (gm_t *)calloc(sizeof(gm_t), 1);
    if (gfxctx == NULL)
    {
        printf("Unable to allocate memory.\n");
        return -1;
    }

    gm_init(gfxctx);

    // initialize console
    gm_console_t *console = NULL;
    if (gm_console_init(&console))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize console.\n");
        gm_shutdown(gfxctx);
        free(gfxctx);
        return 1;
    }
    gm_console_add_text(console, "Console initialized. Press ` to toggle.");

    gm_lua_t *lua_ctx = NULL;
    gm_lua_error_t err = gm_lua_init(&lua_ctx, gfxctx->pixels, gfxctx->cvs_width, gfxctx->cvs_height);
    if (err.code > 100)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize Lua context: %s\n", err.message);
        gm_console_add_text(console, err.message);
        gm_console_show(console);
        gm_shutdown(gfxctx);
        free(gfxctx);
        return 1;
    }

    gm_fps_t *fps = NULL;
    if (gm_fps_init(&fps))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize FPS tracking.\n");
        gm_console_shutdown(console);
        gm_lua_shutdown(lua_ctx);
        gm_shutdown(gfxctx);
        free(gfxctx);
        return 1;
    }

    // load the initial script (if any)
    gm_lua_load_file(lua_ctx);

    // draw loop
    gm_draw(gfxctx, lua_ctx, fps, console);

    gm_lua_shutdown(lua_ctx);
    gm_shutdown(gfxctx);
    gm_fps_shutdown(fps);
    gm_console_shutdown(console);

    free(gfxctx);
    return 0;
}

int gm_init(gm_t *gfxlc)
{
    memset(gfxlc, 0, sizeof(gm_t));

    // dimensions of the canvas
    gfxlc->cvs_width = CNV_W;
    gfxlc->cvs_height = CNV_H;
    gfxlc->cvs_on_win_rect.x = 0;
    gfxlc->cvs_on_win_rect.y = 0;
    gfxlc->cvs_on_win_rect.w = GFX_W;
    gfxlc->cvs_on_win_rect.h = GFX_H;

    // dimensions of the window
    gfxlc->win_width = GFX_W;
    gfxlc->win_height = GFX_H;

    gfxlc->pixels = malloc(gfxlc->cvs_width * gfxlc->cvs_height * sizeof(uint32_t));
    if (!gfxlc->pixels)
    {
        SDL_Log("out of memory\n");
        exit(1);
    }
    memset(gfxlc->pixels, 0, gfxlc->cvs_width * gfxlc->cvs_height * sizeof(uint32_t));

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL Init failed.\n");
        return 1;
    }
    SDL_Log("SDL Init succeeded.\n");

    SDL_srand((unsigned int)time(NULL));

    // create a window with the given dimensions and title
    gfxlc->window = SDL_CreateWindow("gfxlc", gfxlc->win_width, gfxlc->win_height, 0);
    if (gfxlc->window == NULL)
    {
        SDL_Log("Could not get window... %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    load_fonts(gfxlc);

    // create renderer
    gfxlc->renderer = SDL_CreateRenderer(gfxlc->window, NULL);
    if (!gfxlc->renderer)
    {
        SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }

    // create the texture
    gfxlc->texture = SDL_CreateTexture(
        gfxlc->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        gfxlc->cvs_width,
        gfxlc->cvs_height);

    if (!gfxlc->texture)
    {
        SDL_Log("failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    // Set scale mode to NEAREST to get sharp pixel edges
    SDL_SetTextureScaleMode(gfxlc->texture, SDL_SCALEMODE_PIXELART);

    // init game loop vars
    gfxlc->quit = 0;
    gfxlc->start_ticks = SDL_GetTicks();

    return 0;
}

int gm_draw(gm_t *gfxlc, gm_lua_t *lua_ctx, gm_fps_t *fps, gm_console_t *console)
{
    uint64_t prev = SDL_GetTicks();
    while (gfxlc->quit == 0)
    {
        // lua hot reload
        gm_lua_error_t err = gm_lua_hot_reload(lua_ctx);
        if (err.code != 0)
        {
            SDL_Log("Lua hot reload error: %s\n", err.message);
            gm_console_add_text(console, err.message);
            gm_console_show(console);
        }
        else if (err.reloaded && gm_console_shown(console))
        {
            gm_console_add_text(console, "Lua script reloaded successfully.");
            gm_console_hide(console);
        }

        uint64_t now = SDL_GetTicks();
        float dt = (float)(now - prev);
        if (!gm_lua_call_draw(lua_ctx, dt))
        {
            break;
        }
        prev = now;

        SDL_UpdateTexture(
            gfxlc->texture,
            NULL,
            gfxlc->pixels,
            gfxlc->cvs_width * (int)sizeof(uint32_t));

        SDL_SetRenderDrawColor(gfxlc->renderer, 0, 0, 10, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gfxlc->renderer);

        SDL_RenderTexture(gfxlc->renderer, gfxlc->texture, NULL, (const SDL_FRect *)&(gfxlc->cvs_on_win_rect));
        gm_fps_draw(fps, gfxlc->renderer, gfxlc->font, 10, 10);
        gm_console_draw(console, gfxlc->renderer);
        SDL_RenderPresent(gfxlc->renderer);

        while (SDL_PollEvent(&gfxlc->evt))
        {
            if (gfxlc->evt.type == SDL_EVENT_QUIT)
            {
                gfxlc->quit = 1;
            }

            // Check if the pressed key is Escape
            if (gfxlc->evt.key.key == SDLK_ESCAPE)
            {
                SDL_Log("Escape key pressed, quitting.");
                // Set your loop condition to false
                gfxlc->quit = 1;
            }

            // Check if the pressed key is the backtick (`) key
            if (gfxlc->evt.type == SDL_EVENT_KEY_DOWN)
            {

                if (gfxlc->evt.key.key == SDLK_GRAVE)
                {
                    bool console_shown = gm_console_toggle(console);
                    SDL_Log("Toggled console, now %s", console_shown ? "hidden" : "shown");
                }
            }
        }
    }
    return 0;
}

void gm_shutdown(gm_t *gfxlc)
{
    if (gfxlc->texture)
    {
        SDL_DestroyTexture(gfxlc->texture);
    }
    if (gfxlc->renderer)
    {
        SDL_DestroyRenderer(gfxlc->renderer);
    }
    if (gfxlc->window)
    {
        SDL_DestroyWindow(gfxlc->window);
    }

    if (gfxlc->font)
    {
        TTF_CloseFont(gfxlc->font);
    }
    TTF_Quit();

    if (gfxlc->pixels)
    {
        free(gfxlc->pixels);
    }

    SDL_Quit();
}

int load_fonts(gm_t *gfxlc)
{
    if (!TTF_Init())
    {
        SDL_Log("Couldn't initialize SDL_ttf: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const char *base_path = SDL_GetBasePath();
    if (base_path)
    {
        char font_path[1024];

        SDL_snprintf(font_path, sizeof(font_path), "%s/SourceCodePro-Regular.ttf", base_path);

        gfxlc->font = TTF_OpenFont(font_path, 10.0f);
        if (gfxlc->font == NULL)
        {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return -1;
        }
        SDL_Log("Loading font from: %s\n", font_path);
    }
    return 0;
}