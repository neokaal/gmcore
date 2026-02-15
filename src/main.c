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

int gm_sdl_init(gm_t *gmctx);
void gm_sdl_shutdown(gm_t *gmctx);
int gm_sdl_load_fonts(gm_t *gmctx);

int main(int argc, char *argv[])
{
    // 1. Allocate and initialize the game context
    gm_t *gmctx = (gm_t *)calloc(sizeof(gm_t), 1);
    if (gmctx == NULL)
    {
        printf("Unable to allocate memory.\n");
        return -1;
    }

    gm_sdl_init(gmctx);

    // 2. Initialize the on-screen console
    gm_console_t *console = NULL;
    if (gm_console_init(&console))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize console.\n");
        gm_sdl_shutdown(gmctx);
        free(gmctx);
        return 1;
    }
    gm_console_add_text(console, "Console initialized. Press ` to toggle.");

    // 3. Initialize the Lua Bindings
    gm_lua_t *lua_ctx = NULL;
    gm_lua_error_t err = gm_lua_init(&lua_ctx, gmctx->pixels, gmctx->cvs_width, gmctx->cvs_height);
    if (err.code > 100)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize Lua context: %s\n", err.message);
        gm_console_add_text(console, err.message);
        gm_console_show(console);
        gm_sdl_shutdown(gmctx);
        free(gmctx);
        return 1;
    }

    // 4. Initialize the FPS display
    gm_fps_t *fps = NULL;
    if (gm_fps_init(&fps))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize FPS tracking.\n");
        gm_console_shutdown(console);
        gm_lua_shutdown(lua_ctx);
        gm_sdl_shutdown(gmctx);
        free(gmctx);
        return 1;
    }

    // 5. load the Lua game program
    gm_lua_load_file(lua_ctx);

    // 6. Enter the draw loop
    uint64_t prev = SDL_GetTicks();
    while (gmctx->quit == 0)
    {
        // 1. Hot reload the Lua game program
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

        // 2. Call the draw function in the game program
        uint64_t now = SDL_GetTicks();
        float dt = (float)(now - prev);
        if (!gm_lua_call_draw(lua_ctx, dt))
        {
            break;
        }
        prev = now;

        // 3. Update the output texture with the pixels data from game program
        SDL_UpdateTexture(
            gmctx->texture,
            NULL,
            gmctx->pixels,
            gmctx->cvs_width * (int)sizeof(uint32_t));

        // 4. Clear renderer with a dark colour
        SDL_SetRenderDrawColor(gmctx->renderer, 0, 0, 10, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(gmctx->renderer);

        // 5. Draw the game
        SDL_RenderTexture(gmctx->renderer, gmctx->texture, NULL, (const SDL_FRect *)&(gmctx->cvs_on_win_rect));

        // 6. Draw the fps
        gm_fps_draw(fps, gmctx->renderer, gmctx->font, 10, 10);

        // 7. Draw the console
        gm_console_draw(console, gmctx->renderer);

        // 8. Show the screen
        SDL_RenderPresent(gmctx->renderer);

        // 9. Handle the events generated
        while (SDL_PollEvent(&gmctx->evt))
        {
            if (gmctx->evt.type == SDL_EVENT_QUIT)
            {
                gmctx->quit = 1;
            }

            // Check if the pressed key is Escape
            if (gmctx->evt.key.key == SDLK_ESCAPE)
            {
                SDL_Log("Escape key pressed, quitting.");
                // Set your loop condition to false
                gmctx->quit = 1;
            }

            // Check if the pressed key is the backtick (`) key
            if (gmctx->evt.type == SDL_EVENT_KEY_DOWN)
            {

                if (gmctx->evt.key.key == SDLK_GRAVE)
                {
                    bool console_shown = gm_console_toggle(console);
                    SDL_Log("Toggled console, now %s", console_shown ? "hidden" : "shown");
                }
            }
        }
    }

    // 7. Shutdown and exit
    gm_lua_shutdown(lua_ctx);
    gm_sdl_shutdown(gmctx);
    gm_fps_shutdown(fps);
    gm_console_shutdown(console);
    free(gmctx);

    return 0;
}

int gm_sdl_init(gm_t *gmctx)
{
    memset(gmctx, 0, sizeof(gm_t));

    // dimensions of the canvas
    gmctx->cvs_width = CNV_W;
    gmctx->cvs_height = CNV_H;
    gmctx->cvs_on_win_rect.x = 0;
    gmctx->cvs_on_win_rect.y = 0;
    gmctx->cvs_on_win_rect.w = GFX_W;
    gmctx->cvs_on_win_rect.h = GFX_H;

    // dimensions of the window
    gmctx->win_width = GFX_W;
    gmctx->win_height = GFX_H;

    gmctx->pixels = malloc(gmctx->cvs_width * gmctx->cvs_height * sizeof(uint32_t));
    if (!gmctx->pixels)
    {
        SDL_Log("out of memory\n");
        exit(1);
    }
    memset(gmctx->pixels, 0, gmctx->cvs_width * gmctx->cvs_height * sizeof(uint32_t));

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("SDL Init failed.\n");
        return 1;
    }
    SDL_Log("SDL Init succeeded.\n");

    SDL_srand((unsigned int)time(NULL));

    // create a window with the given dimensions and title
    gmctx->window = SDL_CreateWindow("GMCORE", gmctx->win_width, gmctx->win_height, 0);
    if (gmctx->window == NULL)
    {
        SDL_Log("Could not get window... %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    gm_sdl_load_fonts(gmctx);

    // create renderer
    gmctx->renderer = SDL_CreateRenderer(gmctx->window, NULL);
    if (!gmctx->renderer)
    {
        SDL_Log("SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }
    else
    {
        // Enable VSync
        if (SDL_SetRenderVSync(gmctx->renderer, 1) == false)
        {
            SDL_Log("Could not enable VSync! SDL error: %s\n", SDL_GetError());
            exit(1);
        }
    }

    // create the texture
    gmctx->texture = SDL_CreateTexture(
        gmctx->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        gmctx->cvs_width,
        gmctx->cvs_height);

    if (!gmctx->texture)
    {
        SDL_Log("failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    // Set scale mode to NEAREST to get sharp pixel edges
    SDL_SetTextureScaleMode(gmctx->texture, SDL_SCALEMODE_PIXELART);

    // init game loop vars
    gmctx->quit = 0;
    gmctx->start_ticks = SDL_GetTicks();

    return 0;
}

void gm_sdl_shutdown(gm_t *gmctx)
{
    if (gmctx->texture)
    {
        SDL_DestroyTexture(gmctx->texture);
    }
    if (gmctx->renderer)
    {
        SDL_DestroyRenderer(gmctx->renderer);
    }
    if (gmctx->window)
    {
        SDL_DestroyWindow(gmctx->window);
    }

    if (gmctx->font)
    {
        TTF_CloseFont(gmctx->font);
    }
    TTF_Quit();

    if (gmctx->pixels)
    {
        free(gmctx->pixels);
    }

    SDL_Quit();
}

int gm_sdl_load_fonts(gm_t *gmctx)
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

        gmctx->font = TTF_OpenFont(font_path, 10.0f);
        if (gmctx->font == NULL)
        {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return -1;
        }
        SDL_Log("Loading font from: %s\n", font_path);
    }
    return 0;
}