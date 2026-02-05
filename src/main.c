// minimal SDL3 program
#include <stdio.h>
#include <SDL3/SDL.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define GFX_W 800
#define GFX_H 600

lua_State *lua_init_and_load(const char *filename)
{
    lua_State *L = luaL_newstate();
    if (!L)
    {
        fprintf(stderr, "failed to create lua state\n");
        return NULL;
    }

    /* open base + math only */
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);

    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);

    /* load file */
    if (luaL_loadfile(L, filename) != LUA_OK)
    {
        fprintf(stderr, "lua load error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        fprintf(stderr, "lua runtime error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }

    /* verify frame() */
    lua_getglobal(L, "frame");
    if (!lua_isfunction(L, -1))
    {
        fprintf(stderr, "error: script must define frame(t)\n");
        lua_close(L);
        return NULL;
    }
    lua_pop(L, 1);

    return L;
}

int lua_call_frame(lua_State *L, double t)
{
    lua_getglobal(L, "frame");
    lua_pushnumber(L, t);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        fprintf(stderr, "lua frame error: %s\n", lua_tostring(L, -1));
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s script.lua\n", argv[0]);
        return 1;
    }

    lua_State *L = lua_init_and_load(argv[1]);
    if (!L)
    {
        return 1;
    }

    // dimensions of the window
    const int SCREEN_WIDTH = GFX_W;
    const int SCREEN_HEIGHT = GFX_H;

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL Init failed.\n");
        return 1;
    }
    printf("SDL Init succeeded.\n");

    // create a window with the given dimensions and title
    SDL_Window *window = SDL_CreateWindow("gfxlc",
                                          SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (window == NULL)
    {
        printf("Could not get window... %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    // texture related stuff
    uint32_t *pixels;
    SDL_Texture *texture;
    SDL_Renderer *renderer;

    pixels = malloc(GFX_W * GFX_H * sizeof(uint32_t));
    if (!pixels)
    {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memset(pixels, 0, GFX_W * GFX_H * sizeof(uint32_t));

    // create renderer
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
        exit(1);
    }

    // create the texture
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        GFX_W,
        GFX_H);

    if (!texture)
    {
        fprintf(stderr, "failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    // application/game loop
    int quit = 0;
    SDL_Event evt;
    uint64_t start_ticks = SDL_GetTicks();

    while (quit == 0)
    {
        // update
        uint64_t now = SDL_GetTicks();
        double t = (now - start_ticks) / 1000.0;
        if (!lua_call_frame(L, t))
        {
            break;
        }

        // draw
        SDL_UpdateTexture(
            texture,
            NULL,
            pixels,
            GFX_W * sizeof(uint32_t));

        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        // handle messages/events

        while (SDL_PollEvent(&evt))
        {
            if (evt.type == SDL_EVENT_QUIT)
            {
                quit = 1;
            }
        }
    }

    // cleanup
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
