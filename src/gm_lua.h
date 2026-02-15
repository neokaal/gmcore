#ifndef __GM_LUABIND_H__
#define __GM_LUABIND_H__

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#include "gm_util.h"

#define GM_GAME_MT "gfxlc.gm"

typedef struct
{
    uint32_t *pixels;
    int w;
    int h;
    bool stop_running;
} gm_lua_game_t;

typedef struct
{
    // Lua state and script info
    lua_State *L;
    char *lua_file;
    time_t lua_last_mtime;
    gm_lua_game_t *gm;
} gm_lua_t;

typedef struct
{
    int code;
    bool reloaded;
    char message[256];
} gm_lua_error_t;

static inline uint32_t
pack_rgba(int r, int g, int b, int a);

gm_lua_error_t gm_lua_init(gm_lua_t **lua_ctx, uint32_t *pixels, int width, int height);
void gm_lua_shutdown(gm_lua_t *lua_ctx);
gm_lua_error_t gm_lua_load_file(gm_lua_t *lua_ctx);
gm_lua_error_t gm_lua_call_draw(gm_lua_t *lua_ctx, float t);
gm_lua_error_t gm_lua_hot_reload(gm_lua_t *lua_ctx);

static gm_lua_game_t *gm_lua_check_game(lua_State *L);
static int gm_lua_game_clear(lua_State *L);
static int gm_lua_game_noloop(lua_State *L);
static int gm_lua_game_set_pixel(lua_State *L);
static int gm_lua_game_fill_rect(lua_State *L);
static int gm_lua_game_save_pixels_to_image(lua_State *L);
int gm_lua_register_game_api(gm_lua_t *lua_ctx, uint32_t *pixels, int width, int height);

#endif // __GM_LUABIND_H__
