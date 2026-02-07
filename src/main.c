// minimal SDL3 program
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// statusbar height
#define SB_H 16

#define GFX_W 400
#define GFX_H 360 - SB_H

#define GFXLC_CANVAS_MT "gfxlc.canvas"

typedef struct
{
    uint32_t *pixels;
    int w;
    int h;
} lua_canvas_t;

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
lua_State *lua_init();
int load_fonts(gfxlc_t *gfxlc);
void init_fps_texture(gfxlc_t *gfxlc);
void draw_fps(gfxlc_t *gfxlc);
void lua_load_file(lua_State *L, const char *filename);
int lua_call_draw(lua_State *L, float t);
static inline uint32_t pack_rgba(int r, int g, int b, int a);
static lua_canvas_t *check_canvas(lua_State *L);
static int lua_canvas_clear(lua_State *L);
static int lua_canvas_set_pixel(lua_State *L);
static int lua_canvas_fill_rect(lua_State *L);
static int register_canvas_api(lua_State *L, gfxlc_t *gfxlc);
static time_t get_file_mtime(const char *path);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        SDL_Log("usage: %s script.lua\n", argv[0]);
        return 1;
    }

    char *lua_file = argv[1];
    gfxlc_t *gfxctx = (gfxlc_t *)calloc(sizeof(gfxlc_t), 1);
    if (gfxctx == NULL)
    {
        printf("Unable to allocate memory.\n");
        return -1;
    }

    gfxlc_init(gfxctx, lua_file);
    gfxlc_draw(gfxctx);
    gfxlc_shutdown(gfxctx);

    free(gfxctx);
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

    gfxlc->L = lua_init();
    if (!gfxlc->L)
    {
        return 1;
    }
    // register canvas API and load the initial script
    register_canvas_api(gfxlc->L, gfxlc);
    // load the initial script (if any)
    lua_load_file(gfxlc->L, gfxlc->lua_file);

    gfxlc->lua_last_mtime = get_file_mtime(gfxlc->lua_file);

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
        }

        uint64_t now = SDL_GetTicks();
        float dt = (float)(now - prev);
        if (!lua_call_draw(gfxlc->L, dt))
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
        }
    }
    return 0;
}

lua_State *lua_init(void)
{
    lua_State *L = luaL_newstate();
    if (!L)
    {
        SDL_Log("failed to create lua state\n");
        return NULL;
    }

    /* open base + math + table */
    luaL_requiref(L, "_G", luaopen_base, 1);
    lua_pop(L, 1);

    luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(L, 1);

    luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(L, 1);

    return L;
}

void gfxlc_shutdown(gfxlc_t *gfxlc)
{
    if (gfxlc->L)
    {
        lua_close(gfxlc->L);
    }

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
    if (gfxlc->lua_file)
    {
        free(gfxlc->lua_file);
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

        gfxlc->font = TTF_OpenFont(font_path, 16.0f);
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

void lua_load_file(lua_State *L, const char *filename)
{
    if (luaL_loadfile(L, filename) != LUA_OK)
    {
        SDL_Log("lua load error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }

    if (lua_pcall(L, 0, 0, 0) != LUA_OK)
    {
        SDL_Log("lua runtime error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
        return;
    }

    lua_getglobal(L, "draw");
    if (!lua_isfunction(L, -1))
    {
        SDL_Log("error: script must define draw(t)\n");
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
        lua_pop(L, 1);
        return 0;
    }

    return 1;
}

static time_t get_file_mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0)
    {
        return 0;
    }
    return st.st_mtime;
}

static inline uint32_t pack_rgba(int r, int g, int b, int a)
{
    return ((uint32_t)(r & 0xFF) << 24) |
           ((uint32_t)(g & 0xFF) << 16) |
           ((uint32_t)(b & 0xFF) << 8) |
           (uint32_t)(a & 0xFF);
}

static lua_canvas_t *check_canvas(lua_State *L)
{
    return (lua_canvas_t *)luaL_checkudata(L, 1, GFXLC_CANVAS_MT);
}

static int lua_canvas_clear(lua_State *L)
{
    lua_canvas_t *canvas = check_canvas(L);
    int r = luaL_checkinteger(L, 2);
    int g = luaL_checkinteger(L, 3);
    int b = luaL_checkinteger(L, 4);
    int a = 255;
    if (lua_gettop(L) >= 5)
    {
        a = luaL_checkinteger(L, 5);
    }

    uint32_t color = pack_rgba(r, g, b, a);
    int count = canvas->w * canvas->h;
    for (int i = 0; i < count; ++i)
    {
        canvas->pixels[i] = color;
    }

    return 0;
}

static int lua_canvas_set_pixel(lua_State *L)
{
    lua_canvas_t *canvas = check_canvas(L);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int r = luaL_checkinteger(L, 4);
    int g = luaL_checkinteger(L, 5);
    int b = luaL_checkinteger(L, 6);
    int a = 255;
    if (lua_gettop(L) >= 7)
    {
        a = luaL_checkinteger(L, 7);
    }

    if (x < 0 || x >= canvas->w || y < 0 || y >= canvas->h)
    {
        return 0;
    }

    canvas->pixels[y * canvas->w + x] = pack_rgba(r, g, b, a);
    return 0;
}

static int lua_canvas_fill_rect(lua_State *L)
{
    lua_canvas_t *canvas = check_canvas(L);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);
    int r = luaL_checkinteger(L, 6);
    int g = luaL_checkinteger(L, 7);
    int b = luaL_checkinteger(L, 8);
    int a = 255;
    if (lua_gettop(L) >= 9)
    {
        a = luaL_checkinteger(L, 9);
    }

    if (w <= 0 || h <= 0)
    {
        return 0;
    }

    int x0 = x;
    int y0 = y;
    int x1 = x + w;
    int y1 = y + h;

    if (x0 < 0)
    {
        x0 = 0;
    }
    if (y0 < 0)
    {
        y0 = 0;
    }
    if (x1 > canvas->w)
    {
        x1 = canvas->w;
    }
    if (y1 > canvas->h)
    {
        y1 = canvas->h;
    }

    if (x0 >= x1 || y0 >= y1)
    {
        return 0;
    }

    uint32_t color = pack_rgba(r, g, b, a);
    for (int row = y0; row < y1; ++row)
    {
        uint32_t *dst = canvas->pixels + row * canvas->w + x0;
        for (int col = x0; col < x1; ++col)
        {
            *dst++ = color;
        }
    }

    return 0;
}

static int register_canvas_api(lua_State *L, gfxlc_t *gfxlc)
{
    luaL_newmetatable(L, GFXLC_CANVAS_MT);

    lua_newtable(L);
    lua_pushcfunction(L, lua_canvas_clear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, lua_canvas_fill_rect);
    lua_setfield(L, -2, "fill_rect");
    lua_pushcfunction(L, lua_canvas_set_pixel);
    lua_setfield(L, -2, "set_pixel");
    lua_pushinteger(L, gfxlc->cvs_width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, gfxlc->cvs_height);
    lua_setfield(L, -2, "height");
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    lua_canvas_t *canvas = (lua_canvas_t *)lua_newuserdata(L, sizeof(lua_canvas_t));
    canvas->pixels = gfxlc->pixels;
    canvas->w = gfxlc->cvs_width;
    canvas->h = gfxlc->cvs_height;

    luaL_getmetatable(L, GFXLC_CANVAS_MT);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "canvas");

    return 0;
}