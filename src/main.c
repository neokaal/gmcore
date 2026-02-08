// minimal SDL3 program
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "gfxlc_util.h"
#include "gfxlc_context.h"
#include "gfxlc_luabind.h"

// statusbar height
#define SB_H 16

#define GFX_W 400
#define GFX_H 360 - SB_H

int gfxlc_init(gfxlc_t *gfxlc);
int gfxlc_draw(gfxlc_t *gfxlc, gfxlc_lua_t *lua_ctx);
void gfxlc_shutdown(gfxlc_t *gfxlc);
int load_fonts(gfxlc_t *gfxlc);
void init_fps_texture(gfxlc_t *gfxlc);
void draw_fps(gfxlc_t *gfxlc);

int main(int argc, char *argv[])
{
    char *lua_file = NULL;

    // Get the current directory, to load game.lua if found in the directory
    const char *current_dir = SDL_GetCurrentDirectory();
    SDL_Log("Current directory: %s\n", current_dir);

    // If the file "game.lua" exists we are good to go
    if (file_exists("game.lua"))
    {
        SDL_Log("Found game.lua in current directory, loading it.\n");
        lua_file = "game.lua";
    }

    // If there is no game.lua file we print an error and get out
    if (lua_file == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No game.lua file found in the current directory.\n");
        return 1;
    }

    gfxlc_t *gfxctx = (gfxlc_t *)calloc(sizeof(gfxlc_t), 1);
    if (gfxctx == NULL)
    {
        printf("Unable to allocate memory.\n");
        return -1;
    }

    gfxlc_init(gfxctx);

    gfxlc_lua_t *lua_ctx = NULL;
    if (!gfxlc_lua_init(&lua_ctx, lua_file, gfxctx->pixels, gfxctx->cvs_width, gfxctx->cvs_height))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize Lua context.\n");
        gfxlc_shutdown(gfxctx);
        free(gfxctx);
        return 1;
    }

    // load the initial script (if any)
    gfxlc_lua_load_file(lua_ctx);

    gfxlc_draw(gfxctx, lua_ctx);

    gfxlc_lua_shutdown(lua_ctx);
    gfxlc_shutdown(gfxctx);

    free(gfxctx);
    return 0;
}

int gfxlc_init(gfxlc_t *gfxlc)
{
    memset(gfxlc, 0, sizeof(gfxlc_t));

    // dimensions of the canvas
    gfxlc->cvs_width = GFX_W;
    gfxlc->cvs_height = GFX_H;
    gfxlc->cvs_on_win_rect.x = 0;
    gfxlc->cvs_on_win_rect.y = 0;
    gfxlc->cvs_on_win_rect.w = gfxlc->cvs_width;
    gfxlc->cvs_on_win_rect.h = gfxlc->cvs_height;

    // dimensions of the window
    gfxlc->win_width = GFX_W;
    gfxlc->win_height = GFX_H + SB_H;

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
    init_fps_texture(gfxlc);

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

    // init game loop vars
    gfxlc->quit = 0;
    gfxlc->start_ticks = SDL_GetTicks();

    return 0;
}

int gfxlc_draw(gfxlc_t *gfxlc, gfxlc_lua_t *lua_ctx)
{
    uint64_t prev = SDL_GetTicks();
    while (gfxlc->quit == 0)
    {
        // lua hot reload
        gfxlc_lua_hot_reload(lua_ctx);

        uint64_t now = SDL_GetTicks();
        float dt = (float)(now - prev);
        if (!gfxlc_lua_call_draw(lua_ctx, dt))
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
        draw_fps(gfxlc);
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
        }
    }
    return 0;
}

void gfxlc_shutdown(gfxlc_t *gfxlc)
{
    if (gfxlc->fpsTexture)
    {
        SDL_DestroyTexture(gfxlc->fpsTexture);
    }

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

int load_fonts(gfxlc_t *gfxlc)
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

void init_fps_texture(gfxlc_t *gfxlc)
{
    gfxlc->lastUpdateTime = 0;
    gfxlc->frameCount = 0;
    gfxlc->fpsTexture = NULL;
    gfxlc->fpsRect.x = gfxlc->win_width - 100;
    gfxlc->fpsRect.y = gfxlc->win_height - SB_H + 4;
    gfxlc->fpsRect.w = 100;
    gfxlc->fpsRect.h = SB_H - 8;
    gfxlc->currentFPS = 0.0f;
}

void draw_fps(gfxlc_t *gfxlc)
{
    uint64_t currentTime = SDL_GetTicks();
    gfxlc->frameCount++;

    if (currentTime > gfxlc->lastUpdateTime + 500)
    {
        float elapsedSeconds = (currentTime - gfxlc->lastUpdateTime) / 1000.0f;
        gfxlc->currentFPS = (elapsedSeconds > 0.0f) ? (gfxlc->frameCount / elapsedSeconds) : 0.0f;
        gfxlc->frameCount = 0;
        gfxlc->lastUpdateTime = currentTime;

        if (gfxlc->fpsTexture)
        {
            SDL_DestroyTexture(gfxlc->fpsTexture);
        }

        char text[32];
        snprintf(text, sizeof(text), "FPS: %.2f", gfxlc->currentFPS);
        SDL_Color fg = {255, 255, 255, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(gfxlc->font, text, 0, fg);
        if (surf)
        {
            gfxlc->fpsTexture = SDL_CreateTextureFromSurface(gfxlc->renderer, surf);
            gfxlc->fpsRect.w = (float)surf->w;
            gfxlc->fpsRect.h = (float)surf->h;
            SDL_DestroySurface(surf);
        }
    }

    if (gfxlc->fpsTexture)
    {
        SDL_RenderTexture(gfxlc->renderer, gfxlc->fpsTexture, NULL, &gfxlc->fpsRect);
    }
}
