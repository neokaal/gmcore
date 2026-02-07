// minimal SDL3 program
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// statusbar height
#define SB_H 32

#define GFX_W 800
#define GFX_H 600 - SB_H

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

    // fps texture related state
    int frameCount;
    uint64_t lastUpdateTime;
    SDL_Texture *fpsTexture;
    SDL_FRect fpsRect;
    float currentFPS;
} gfxlc_t;

int gfxlc_init(gfxlc_t *gfxlc, const char *lua_file);

int gfxlc_draw(gfxlc_t *gfxlc);

void gfxlc_shutdown(gfxlc_t *gfxlc);

lua_State *lua_init_and_load(const char *filename);

int load_fonts(gfxlc_t *gfxlc);

void init_fps_texture(gfxlc_t *gfxlc);

void draw_fps(gfxlc_t *gfxlc);

void lua_load_file(lua_State *L, const char *filename);

int lua_call_draw(lua_State *L, float t);

static uint32_t *get_pixels(lua_State *L);

static int lua_set_pixel(lua_State *L);

static int gfxlc_lua_blit(lua_State *L);

static int lua_bg(lua_State *L);

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

    lua_pushcfunction(gfxlc->L, gfxlc_lua_blit);
    lua_setglobal(gfxlc->L, "blit");

    lua_pushcfunction(gfxlc->L, lua_bg);
    lua_setglobal(gfxlc->L, "bg");

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

    // load fonts
    load_fonts(gfxlc);

    init_fps_texture(gfxlc);

    // init the fps texture
    init_fps_texture(gfxlc);

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
    uint64_t prev = SDL_GetTicks();
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
        float dt = (now - prev);
        if (!lua_call_draw(gfxlc->L, dt))
        {
            break;
        }
        prev = now;

        // draw
        SDL_UpdateTexture(
            gfxlc->texture,
            NULL,
            gfxlc->pixels,
            GFX_W * sizeof(uint32_t));

        SDL_SetRenderDrawColor(gfxlc->renderer, 0, 0, 10, SDL_ALPHA_OPAQUE);

        SDL_RenderClear(gfxlc->renderer);

        SDL_RenderTexture(gfxlc->renderer, gfxlc->texture, NULL, (const SDL_FRect *)&(gfxlc->cvs_on_win_rect));

        draw_fps(gfxlc);

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
        SDL_Log("failed to create lua state\n");
        return NULL;
    }

    /* open base + math only */
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);

    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);

    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);

    /* load file */
    if (luaL_loadfile(L, filename) != LUA_OK)
    {
        SDL_Log("lua load error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        SDL_Log("lua runtime error: %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }

    /* verify draw() */
    lua_getglobal(L, "draw");
    if (!lua_isfunction(L, -1))
    {
        SDL_Log("error: script must define draw(t)\n");
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

    if (gfxlc->font)
    {
        TTF_CloseFont(gfxlc->font);
    }
    TTF_Quit();

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

        // Load a font
        gfxlc->font = TTF_OpenFont(font_path, 16.0f);
        if (gfxlc->font == NULL)
        {
            SDL_Log("Failed to load font: %s", SDL_GetError());
            return -1;
        }
        SDL_Log("Loading font from: %s\n", font_path);
        //SDL_free(base_path);
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
    // Inside your main loop
    uint64_t currentTime = SDL_GetTicks();
    gfxlc->frameCount++;

    // Only update the texture every 500ms
    if (currentTime > gfxlc->lastUpdateTime + 500)
    {
        // 1. Calculate FPS
        float elapsedSeconds = (currentTime - gfxlc->lastUpdateTime) / 1000.0f;
        gfxlc->currentFPS = gfxlc->frameCount / elapsedSeconds;
        gfxlc->frameCount = 0;
        gfxlc->lastUpdateTime = currentTime;

        // 2. Clean up old texture
        if (gfxlc->fpsTexture)
        {
            SDL_DestroyTexture(gfxlc->fpsTexture);
        }

        // 3. Create new texture
        char text[32];
        snprintf(text, sizeof(text), "FPS: %.2f", gfxlc->currentFPS);
        SDL_Color fg = {255, 255, 255, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(gfxlc->font, text, 0, fg);
        gfxlc->fpsTexture = SDL_CreateTextureFromSurface(gfxlc->renderer, surf);
        gfxlc->fpsRect.w = (float)surf->w;
        gfxlc->fpsRect.h = (float)surf->h;
        SDL_DestroySurface(surf);

        if (gfxlc->fpsTexture)
        {
            SDL_RenderTexture(gfxlc->renderer, gfxlc->fpsTexture, NULL, &gfxlc->fpsRect);
        }

        // SDL_Log("FPS is %s\n", text);
    }

    // 4. Draw the CACHED texture every frame (High performance!)
    if (gfxlc->fpsTexture)
    {
        SDL_RenderTexture(gfxlc->renderer, gfxlc->fpsTexture, NULL, &gfxlc->fpsRect);
    }
}

void lua_load_file(lua_State *L, const char *filename)
{
    if (luaL_loadfile(L, filename) != LUA_OK)
    {
        SDL_Log("lua load error: %s\n", lua_tostring(L, -1));
        return;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        SDL_Log("lua runtime error: %s\n", lua_tostring(L, -1));
        return;
    }

    /* verify draw() */
    lua_getglobal(L, "draw");
    if (!lua_isfunction(L, -1))
    {
        SDL_Log("error: script must define draw(t)\n");
        lua_close(L);
        return;
    }
    lua_pop(L, 1);
}

int lua_call_draw(lua_State *L, float dt)
{
    lua_getglobal(L, "draw");
    lua_pushnumber(L, dt);

    if (lua_pcall(L, 1, 0, 0) != LUA_OK)
    {
        SDL_Log("lua draw error: %s\n", lua_tostring(L, -1));
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

static int gfxlc_lua_blit(lua_State *L)
{
    uint32_t *pixels = get_pixels(L);
    if (!pixels)
    {
        return 0;
    }

    // get the global width and height of the canvas
    lua_getfield(L, LUA_REGISTRYINDEX, "gfx_width");
    int canvas_w = luaL_checkinteger(L, -1);
    lua_pop(L, 1); // pop width
    lua_getfield(L, LUA_REGISTRYINDEX, "gfx_height");
    int canvas_h = luaL_checkinteger(L, -1);
    lua_pop(L, 1); // pop height

    // check table
    if (!lua_istable(L, 1))
    {
        return luaL_error(L, "first argument must be a table");
    }

    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);

    // bounds check
    if (x < 0 || y < 0 || x + w > canvas_w || y + h > canvas_h)
    {
        return luaL_error(L, "dump area out of bounds");
    }

    int table_len = luaL_len(L, 1);
    if (table_len < w * h * 3)
    {
        return luaL_error(L, "image data is smaller than the target dump area");
    }

    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            int idx = (j * w + i) * 3 + 1;
            lua_rawgeti(L, 1, idx);
            lua_rawgeti(L, 1, idx + 1);
            lua_rawgeti(L, 1, idx + 2);

            int r = luaL_checkinteger(L, -3);
            int g = luaL_checkinteger(L, -2);
            int b = luaL_checkinteger(L, -1);
            int a = 255; // default alpha

            // check for alpha value
            if (table_len >= w * h * 4)
            {
                lua_rawgeti(L, 1, idx + 3);
                a = luaL_checkinteger(L, -1);
                lua_pop(L, 1);
            }

            pixels[(y + j) * canvas_w + (x + i)] = (r << 24) | (g << 16) | (b << 8) | (a & 0xFF);

            lua_pop(L, 3);
        }
    }

    return 0;
}


static int lua_bg(lua_State *L)
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

    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
    int b = luaL_checkinteger(L, 3);
    // optional alpha?
    int a = 255;
    if (lua_gettop(L) >= 4)
    {
        a = luaL_checkinteger(L, 4);
    }

    uint32_t color = (r << 24) | (g << 16) | (b << 8) | (a & 0xFF);

    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            pixels[y * w + x] = color;
        }
    }

    return 0;
}
