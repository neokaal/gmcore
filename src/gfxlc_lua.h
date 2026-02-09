#ifndef __GFXLC_LUABIND_H__
#define __GFXLC_LUABIND_H__

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#include "gfxlc_util.h"

#define GFXLC_GAME_MT "gfxlc.gm"

typedef struct
{
    uint32_t *pixels;
    int w;
    int h;
} gfxlc_lua_game_t;

typedef struct
{
    // Lua state and script info
    lua_State *L;
    char *lua_file;
    time_t lua_last_mtime;
} gfxlc_lua_t;

typedef struct
{
    int code;
    bool reloaded;
    char message[256];
} gfxlc_lua_error_t;

static inline uint32_t
pack_rgba(int r, int g, int b, int a);

int gfxlc_lua_init(gfxlc_lua_t **lua_ctx, const char *lua_file, uint32_t *pixels, int width, int height);
void gfxlc_lua_shutdown(gfxlc_lua_t *lua_ctx);
gfxlc_lua_error_t gfxlc_lua_load_file(gfxlc_lua_t *lua_ctx);
int gfxlc_lua_call_draw(gfxlc_lua_t *lua_ctx, float t);
gfxlc_lua_error_t gfxlc_lua_hot_reload(gfxlc_lua_t *lua_ctx);

static gfxlc_lua_game_t *gfxlc_lua_check_game(lua_State *L);
static int gfxlc_lua_game_clear(lua_State *L);
static int gfxlc_lua_game_set_pixel(lua_State *L);
static int gfxlc_lua_game_fill_rect(lua_State *L);
int gfxlc_lua_register_game_api(lua_State *L, uint32_t *pixels, int width, int height);

#endif // __GFXLC_LUABIND_H__
