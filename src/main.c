// minimal SDL3 program
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include <SDL3/SDL.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define GFX_W 800
#define GFX_H 600

typedef struct
{
    // Lua state and script info
    lua_State *L;
    char *lua_file;
    time_t lua_last_mtime;

    // SDL window, renderer, and texture
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    // game loop
    int quit;
    SDL_Event evt;
    uint64_t start_ticks;

    // canvas pixel buffer
    uint32_t *pixels;

    // canvas dimensions
    int cvs_width;
    int cvs_height;

    // window dimensions
    int win_width;
    int win_height;
} gfxlc_t;

int gfxlc_init(gfxlc_t *gfxlc, const char *lua_file);

int gfxlc_draw(gfxlc_t *gfxlc);

void gfxlc_shutdown(gfxlc_t *gfxlc);

lua_State *lua_init_and_load(const char *filename);

void lua_load_file(lua_State *L, const char *filename);

int lua_call_frame(lua_State *L, double t);

static uint32_t *get_pixels(lua_State *L);

static int lua_set_pixel(lua_State *L);

static time_t get_file_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
        return 0;
    return st.st_mtime;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: %s script.lua\n", argv[0]);
        return 1;
    }
    char *lua_file = argv[1];
    gfxlc_t *gfxctx;
    gfxctx = (gfxlc_t *)calloc(sizeof(gfxlc_t), 1);
    if (gfxctx == NULL)
    {
        printf("Unable to allocate memory.\n");
        return -1;
    }

    gfxlc_init(gfxctx, lua_file);
    gfxlc_draw(gfxctx);
    gfxlc_shutdown(gfxctx);

    free(gfxctx);

    // cleanup
    return 0;
}

int gfxlc_init(gfxlc_t *gfxlc, const char *lua_file)
{
    memset(gfxlc, 0, sizeof(gfxlc_t));
    if (lua_file == NULL)
    {
        gfxlc->lua_file = NULL;
    }
    else
    {
        gfxlc->lua_file = strdup(lua_file);
    }

    gfxlc->L = lua_init_and_load(gfxlc->lua_file);

    if (!gfxlc->L)
    {
        return 1;
    }

    gfxlc->lua_last_mtime = get_file_mtime(gfxlc->lua_file);

    lua_pushcfunction(gfxlc->L, lua_set_pixel);
    lua_setglobal(gfxlc->L, "px");

    // dimensions of the canvas
    gfxlc->cvs_width = GFX_W;
    gfxlc->cvs_height = GFX_H;

    // dimensions of the window
    gfxlc->win_width = GFX_W;
    gfxlc->win_height = GFX_H;

    // set values width and height in the global namespace
    // width and height are dimensions of the canvas, not the window
    lua_pushinteger(gfxlc->L, gfxlc->cvs_width);
    lua_setglobal(gfxlc->L, "width");
    lua_pushinteger(gfxlc->L, gfxlc->cvs_height);
    lua_setglobal(gfxlc->L, "height");

    // initialize SDL3
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        printf("SDL Init failed.\n");
        return 1;
    }
    printf("SDL Init succeeded.\n");

    // create a window with the given dimensions and title
    gfxlc->window = SDL_CreateWindow("gfxlc",
                                     gfxlc->win_width, gfxlc->win_height, 0);
    if (gfxlc->window == NULL)
    {
        printf("Could not get window... %s\n", SDL_GetError());
        SDL_Quit();
        return 2;
    }

    gfxlc->pixels = malloc(gfxlc->cvs_width * gfxlc->cvs_height * sizeof(uint32_t));
    if (!gfxlc->pixels)
    {
        fprintf(stderr, "out of memory\n");
        exit(1);
    }
    memset(gfxlc->pixels, 0, gfxlc->cvs_width * gfxlc->cvs_height * sizeof(uint32_t));

    lua_pushlightuserdata(gfxlc->L, gfxlc->pixels);
    lua_setfield(gfxlc->L, LUA_REGISTRYINDEX, "gfx_pixels");

    lua_pushinteger(gfxlc->L, gfxlc->cvs_width);
    lua_setfield(gfxlc->L, LUA_REGISTRYINDEX, "gfx_width");

    lua_pushinteger(gfxlc->L, gfxlc->cvs_height);
    lua_setfield(gfxlc->L, LUA_REGISTRYINDEX, "gfx_height");

    // create renderer
    gfxlc->renderer = SDL_CreateRenderer(gfxlc->window, NULL);
    if (!gfxlc->renderer)
    {
        fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
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
        fprintf(stderr, "failed to create texture: %s\n", SDL_GetError());
        exit(1);
    }

    // init game loop vars
    gfxlc->quit = 0;
    gfxlc->start_ticks = SDL_GetTicks();

    return 0;
}

int gfxlc_draw(gfxlc_t *gfxlc)
{
    while (gfxlc->quit == 0)
    {
        // hot-reload lua script if modified
        time_t mtime = get_file_mtime(gfxlc->lua_file);
        if (mtime != 0 && mtime != gfxlc->lua_last_mtime)
        {
            printf("reloading %s\n", gfxlc->lua_file);

            gfxlc->lua_last_mtime = mtime;

            lua_load_file(gfxlc->L, gfxlc->lua_file);

            if (!gfxlc->L)
            {
                break; /* hard failure */
            }
        }

        // update
        uint64_t now = SDL_GetTicks();
        double t = (now - gfxlc->start_ticks) / 1000.0;
        if (!lua_call_frame(gfxlc->L, t))
        {
            break;
        }

        // draw
        SDL_UpdateTexture(
            gfxlc->texture,
            NULL,
            gfxlc->pixels,
            GFX_W * sizeof(uint32_t));

        SDL_RenderClear(gfxlc->renderer);
        SDL_RenderTexture(gfxlc->renderer, gfxlc->texture, NULL, NULL);
        SDL_RenderPresent(gfxlc->renderer);

        // handle messages/events

        while (SDL_PollEvent(&gfxlc->evt))
        {
            if (gfxlc->evt.type == SDL_EVENT_QUIT)
            {
                gfxlc->quit = 1;
            }
        }
    }
    return 0;
}

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

void gfxlc_shutdown(gfxlc_t *gfxlc)
{
    lua_close(gfxlc->L);
    SDL_DestroyTexture(gfxlc->texture);
    SDL_DestroyRenderer(gfxlc->renderer);
    SDL_DestroyWindow(gfxlc->window);
    SDL_Quit();
}

void lua_load_file(lua_State *L, const char *filename)
{
    if (luaL_loadfile(L, filename) != LUA_OK)
    {
        fprintf(stderr, "lua load error: %s\n", lua_tostring(L, -1));
        return;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        fprintf(stderr, "lua runtime error: %s\n", lua_tostring(L, -1));
        return;
    }

    /* verify frame() */
    lua_getglobal(L, "frame");
    if (!lua_isfunction(L, -1))
    {
        fprintf(stderr, "error: script must define frame(t)\n");
        lua_close(L);
        return;
    }
    lua_pop(L, 1);
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

static uint32_t *get_pixels(lua_State *L)
{
    lua_getfield(L, LUA_REGISTRYINDEX, "gfx_pixels");
    uint32_t *p = lua_touserdata(L, -1);
    lua_pop(L, 1);
    return p;
}

static int lua_set_pixel(lua_State *L)
{
    uint32_t *pixels = get_pixels(L);
    if (!pixels)
    {
        return 0;
    }
    // get the global width and height of the canvas
    lua_getfield(L, LUA_REGISTRYINDEX, "gfx_width");
    int w = luaL_checkinteger(L, -1);
    lua_pop(L, 1); // pop width
    lua_getfield(L, LUA_REGISTRYINDEX, "gfx_height");
    int h = luaL_checkinteger(L, -1);
    lua_pop(L, 1); // pop height

    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    int g = luaL_checkinteger(L, 4);
    int b = luaL_checkinteger(L, 5);
    // optional alpha?
    int a = 255;
    if (lua_gettop(L) >= 6)
    {
        a = luaL_checkinteger(L, 6);
    }

    if (x < 0 || x >= w || y < 0 || y >= h)
    {
        return 0;
    }

    pixels[y * w + x] =
        (r << 24) | (g << 16) | (b << 8) | (a & 0xFF);

    return 0;
}
