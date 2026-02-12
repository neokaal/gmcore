#include <stdlib.h>
#include "gm_lua.h"

static inline uint32_t pack_rgba(int r, int g, int b, int a)
{
    return ((uint32_t)(r & 0xFF) << 24) |
           ((uint32_t)(g & 0xFF) << 16) |
           ((uint32_t)(b & 0xFF) << 8) |
           (uint32_t)(a & 0xFF);
}

gm_lua_error_t gm_lua_init(gm_lua_t **lua_ctx, uint32_t *pixels, int width, int height)
{
    char *lua_file = "game.lua";
    bool file_found = false;
    gm_lua_error_t err;
    err.reloaded = false;
    err.code = 0;
    err.message[0] = '\0';

    // Get the current directory, to load game.lua if found in the directory
    const char *current_dir = SDL_GetCurrentDirectory();
    SDL_Log("Current directory: %s\n", current_dir);

    // If the file "game.lua" exists we are good to go
    if (file_exists("game.lua"))
    {
        SDL_Log("Found game.lua in current directory, loading it.\n");
        // log to console as well
        err.code = 10;
        snprintf(err.message, sizeof(err.message), "Found game.lua in current directory, loading it.");
        file_found = true;
    }

    // If there is no game.lua file we print an error and get out
    if (!file_found)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "No game.lua file found in the current directory.\n");
        err.code = 11;
        snprintf(err.message, sizeof(err.message), "No game.lua file found in the current directory. Create one.");
    }

    (*lua_ctx) = (gm_lua_t *)calloc(sizeof(gm_lua_t), 1);
    if ((*lua_ctx) == NULL)
    {
        SDL_Log("Unable to allocate memory for gm_lua_t.\n");
        err.code = 100;
        snprintf(err.message, sizeof(err.message), "Unable to allocate memory for Lua context.");
    }

    gm_lua_t *lc = (*lua_ctx);

    lc->gm = NULL;
    lc->lua_file = strdup(lua_file);

    lc->L = luaL_newstate();
    if (!lc->L)
    {
        SDL_Log("failed to create lua state\n");
        err.code = 101;
        snprintf(err.message, sizeof(err.message), "Failed to create Lua state.");
        return err;
    }

    /* open base + math + table */
    luaL_requiref(lc->L, "_G", luaopen_base, 1);
    lua_pop(lc->L, 1);

    luaL_requiref(lc->L, LUA_MATHLIBNAME, luaopen_math, 1);
    lua_pop(lc->L, 1);

    luaL_requiref(lc->L, LUA_TABLIBNAME, luaopen_table, 1);
    lua_pop(lc->L, 1);

    // register game API and load the initial script
    gm_lua_register_game_api(lc, pixels, width, height);

    return err;
}

void gm_lua_shutdown(gm_lua_t *lua_ctx)
{
    if (lua_ctx)
    {
        if (lua_ctx->L)
        {
            lua_close(lua_ctx->L);
        }
        if (lua_ctx->lua_file)
        {
            free(lua_ctx->lua_file);
        }
        free(lua_ctx);
    }
}

gm_lua_error_t gm_lua_load_file(gm_lua_t *lua_ctx)
{
    gm_lua_error_t err;
    err.reloaded = false;

    if (luaL_loadfile(lua_ctx->L, lua_ctx->lua_file) != LUA_OK)
    {
        err.code = 1;
        const char *lua_err_msg = lua_tostring(lua_ctx->L, -1);
        snprintf(err.message, sizeof(err.message), "lua load error: %s", lua_err_msg);
        SDL_Log("lua load error: %s\n", lua_err_msg);
        lua_pop(lua_ctx->L, 1);
        return err;
    }

    if (lua_pcall(lua_ctx->L, 0, 0, 0) != LUA_OK)
    {
        err.code = 2;
        const char *lua_err_msg = lua_tostring(lua_ctx->L, -1);
        snprintf(err.message, sizeof(err.message), "lua runtime error: %s", lua_err_msg);
        SDL_Log("lua runtime error: %s\n", lua_err_msg);
        lua_pop(lua_ctx->L, 1);
        return err;
    }

    lua_getglobal(lua_ctx->L, "draw");
    if (!lua_isfunction(lua_ctx->L, -1))
    {
        err.code = 3;
        snprintf(err.message, sizeof(err.message), "script must define draw(t)");

        SDL_Log("error: script must define draw(t)\n");
        lua_pop(lua_ctx->L, 1);
        return err;
    }
    lua_pop(lua_ctx->L, 1);

    lua_ctx->lua_last_mtime = get_file_mtime(lua_ctx->lua_file);
    err.code = 0;
    err.message[0] = '\0';
    err.reloaded = true;
    return err;
}

gm_lua_error_t gm_lua_hot_reload(gm_lua_t *lua_ctx)
{
    // hot-reload lua script if modified and stop_running is false
    time_t mtime = get_file_mtime(lua_ctx->lua_file);
    if (mtime != 0 && mtime != lua_ctx->lua_last_mtime)
    {
        printf("reloading %s\n", lua_ctx->lua_file);

        lua_ctx->lua_last_mtime = mtime;
        return gm_lua_load_file(lua_ctx);
    }
    gm_lua_error_t err;
    err.code = 0;
    err.message[0] = '\0';
    err.reloaded = false;
    return err;
}

int gm_lua_call_draw(gm_lua_t *lua_ctx, float dt)
{
    if (!lua_ctx->gm->stop_running)
    {
        lua_getglobal(lua_ctx->L, "draw");
        lua_pushnumber(lua_ctx->L, dt);

        if (lua_pcall(lua_ctx->L, 1, 0, 0) != LUA_OK)
        {
            SDL_Log("lua draw error: %s\n", lua_tostring(lua_ctx->L, -1));
            lua_pop(lua_ctx->L, 1);
            return 0;
        }
    }

    return 1;
}

static gm_lua_game_t *gm_lua_check_game(lua_State *L)
{
    return (gm_lua_game_t *)luaL_checkudata(L, 1, GM_GAME_MT);
}

static int gm_lua_game_clear(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int r = luaL_checkinteger(L, 2);
    int g = luaL_checkinteger(L, 3);
    int b = luaL_checkinteger(L, 4);
    int a = 255;
    if (lua_gettop(L) >= 5)
    {
        a = luaL_checkinteger(L, 5);
    }

    uint32_t color = pack_rgba(r, g, b, a);
    int count = game->w * game->h;
    for (int i = 0; i < count; ++i)
    {
        game->pixels[i] = color;
    }

    return 0;
}

static int gm_lua_game_set_pixel(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
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

    if (x < 0 || x >= game->w || y < 0 || y >= game->h)
    {
        return 0;
    }

    game->pixels[y * game->w + x] = pack_rgba(r, g, b, a);
    return 0;
}

static int gm_lua_game_fill_rect(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
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
    if (x1 > game->w)
    {
        x1 = game->w;
    }
    if (y1 > game->h)
    {
        y1 = game->h;
    }

    if (x0 >= x1 || y0 >= y1)
    {
        return 0;
    }

    uint32_t color = pack_rgba(r, g, b, a);
    for (int row = y0; row < y1; ++row)
    {
        uint32_t *dst = game->pixels + row * game->w + x0;
        for (int col = x0; col < x1; ++col)
        {
            *dst++ = color;
        }
    }

    return 0;
}

static int gm_lua_game_noloop(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    game->stop_running = true;
    return 0;
}

int gm_lua_register_game_api(gm_lua_t *lua_ctx, uint32_t *pixels, int width, int height)
{
    lua_State *L = lua_ctx->L;

    luaL_newmetatable(L, GM_GAME_MT);

    lua_newtable(L);
    lua_pushcfunction(L, gm_lua_game_clear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, gm_lua_game_noloop);
    lua_setfield(L, -2, "noloop");
    lua_pushcfunction(L, gm_lua_game_fill_rect);
    lua_setfield(L, -2, "fill_rect");
    lua_pushcfunction(L, gm_lua_game_set_pixel);
    lua_setfield(L, -2, "set_pixel");
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    gm_lua_game_t *gm = (gm_lua_game_t *)lua_newuserdata(L, sizeof(gm_lua_game_t));
    gm->pixels = pixels;
    gm->w = width;
    gm->h = height;
    gm->stop_running = false;

    luaL_getmetatable(L, GM_GAME_MT);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "gm");

    lua_ctx->gm = gm;

    return 0;
}
