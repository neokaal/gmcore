#ifndef __GFXLC_LUABIND_H__
#define __GFXLC_LUABIND_H__

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdint.h>
#include <SDL3/SDL.h>

#define GFXLC_GAME_MT "gfxlc.gm"

typedef struct
{
    uint32_t *pixels;
    int w;
    int h;
} lua_game_t;

static inline uint32_t pack_rgba(int r, int g, int b, int a);

lua_State *lua_init();
void lua_load_file(lua_State *L, const char *filename);
int lua_call_draw(lua_State *L, float t);
static lua_game_t *check_game(lua_State *L);
static int lua_game_clear(lua_State *L);
static int lua_game_set_pixel(lua_State *L);
static int lua_game_fill_rect(lua_State *L);
int register_game_api(lua_State *L, uint32_t *pixels, int width, int height);

#endif // __GFXLC_LUABIND_H__
