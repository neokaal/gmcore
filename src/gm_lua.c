#include <stdlib.h>
#include "gm_lua.h"

static inline uint8_t gm_u8_clamp(int v)
{
    if (v < 0)
    {
        return 0;
    }
    if (v > 255)
    {
        return 255;
    }
    return (uint8_t)v;
}

static inline void gm_lua_apply_color(gm_lua_game_t *game, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    game->r = r;
    game->g = g;
    game->b = b;
    game->a = a;
    SDL_SetRenderDrawColor(game->renderer, game->r, game->g, game->b, game->a);
}

static inline void gm_lua_draw_brush(gm_lua_game_t *game, int x, int y)
{
    if (game->line_width <= 1)
    {
        SDL_RenderPoint(game->renderer, (float)x, (float)y);
        return;
    }

    int half = game->line_width / 2;
    SDL_FRect rect = {
        .x = (float)(x - half),
        .y = (float)(y - half),
        .w = (float)game->line_width,
        .h = (float)game->line_width};
    SDL_RenderFillRect(game->renderer, &rect);
}

gm_lua_error_t gm_lua_init(gm_lua_t **lua_ctx, SDL_Renderer *renderer, SDL_Texture *canvas_texture, int width, int height)
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

    // TODO: commented out - required only when debugging
    // luaL_requiref(lc->L, LUA_OSLIBNAME, luaopen_os, 1);
    // lua_pop(lc->L, 1);

    // register game API and load the initial script
    gm_lua_register_game_api(lc, renderer, canvas_texture, width, height);

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

    // mark noloop false
    lua_ctx->gm->stop_running = false;

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

gm_lua_error_t gm_lua_call_draw(gm_lua_t *lua_ctx, float dt)
{
    gm_lua_error_t err;
    err.code = 0;
    err.reloaded = false;
    err.message[0] = '\0';

    if (!lua_ctx->gm->stop_running)
    {
        lua_getglobal(lua_ctx->L, "draw");
        lua_pushnumber(lua_ctx->L, dt);

        if (lua_pcall(lua_ctx->L, 1, 0, 0) != LUA_OK)
        {
            const char *lua_err_msg = lua_tostring(lua_ctx->L, -1);
            snprintf(err.message, sizeof(err.message), "lua runtime error: %s", lua_err_msg);
            err.code = 30;
            SDL_Log("lua draw error: %s\n", lua_err_msg);

            // turn off draw loop till next reload
            lua_ctx->gm->stop_running = true;
            lua_pop(lua_ctx->L, 1);
        }
    }

    return err;
}

static gm_lua_game_t *gm_lua_check_game(lua_State *L)
{
    return (gm_lua_game_t *)luaL_checkudata(L, 1, GM_GAME_MT);
}

static int gm_lua_game_clear(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int argc = lua_gettop(L);

    if (argc >= 4)
    {
        int r = luaL_checkinteger(L, 2);
        int g = luaL_checkinteger(L, 3);
        int b = luaL_checkinteger(L, 4);
        int a = 255;
        if (argc >= 5)
        {
            a = luaL_checkinteger(L, 5);
        }
        gm_lua_apply_color(game, gm_u8_clamp(r), gm_u8_clamp(g), gm_u8_clamp(b), gm_u8_clamp(a));
    }

    SDL_SetRenderDrawColor(game->renderer, game->r, game->g, game->b, game->a);
    SDL_RenderClear(game->renderer);

    return 0;
}

static int gm_lua_game_set_color(lua_State *L)
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

    gm_lua_apply_color(game, gm_u8_clamp(r), gm_u8_clamp(g), gm_u8_clamp(b), gm_u8_clamp(a));
    return 0;
}

static int gm_lua_game_set_line_width(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int w = luaL_checkinteger(L, 2);
    if (w < 1)
    {
        w = 1;
    }
    game->line_width = w;
    return 0;
}

static int gm_lua_game_set_pixel(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int argc = lua_gettop(L);

    if (x < 0 || x >= game->w || y < 0 || y >= game->h)
    {
        return 0;
    }

    if (argc >= 6)
    {
        int r = luaL_checkinteger(L, 4);
        int g = luaL_checkinteger(L, 5);
        int b = luaL_checkinteger(L, 6);
        int a = 255;
        if (argc >= 7)
        {
            a = luaL_checkinteger(L, 7);
        }
        SDL_SetRenderDrawColor(game->renderer, gm_u8_clamp(r), gm_u8_clamp(g), gm_u8_clamp(b), gm_u8_clamp(a));
    }
    else
    {
        SDL_SetRenderDrawColor(game->renderer, game->r, game->g, game->b, game->a);
    }

    gm_lua_draw_brush(game, x, y);
    return 0;
}

static int gm_lua_game_line(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int x1 = luaL_checkinteger(L, 2);
    int y1 = luaL_checkinteger(L, 3);
    int x2 = luaL_checkinteger(L, 4);
    int y2 = luaL_checkinteger(L, 5);
    int argc = lua_gettop(L);

    if (argc >= 8)
    {
        int r = luaL_checkinteger(L, 6);
        int g = luaL_checkinteger(L, 7);
        int b = luaL_checkinteger(L, 8);
        int a = 255;
        if (argc >= 9)
        {
            a = luaL_checkinteger(L, 9);
        }
        SDL_SetRenderDrawColor(game->renderer, gm_u8_clamp(r), gm_u8_clamp(g), gm_u8_clamp(b), gm_u8_clamp(a));
    }
    else
    {
        SDL_SetRenderDrawColor(game->renderer, game->r, game->g, game->b, game->a);
    }

    // Fast path: native line drawing for 1px width.
    if (game->line_width <= 1)
    {
        SDL_RenderLine(game->renderer, (float)x1, (float)y1, (float)x2, (float)y2);
        return 0;
    }

    int dx = x2 - x1;
    int dy = y2 - y1;
    int adx = (dx < 0) ? -dx : dx;
    int ady = (dy < 0) ? -dy : dy;
    int steps = (adx > ady) ? adx : ady;
    if (steps == 0)
    {
        gm_lua_draw_brush(game, x1, y1);
        return 0;
    }

    for (int i = 0; i <= steps; ++i)
    {
        int x = x1 + (dx * i) / steps;
        int y = y1 + (dy * i) / steps;
        gm_lua_draw_brush(game, x, y);
    }
    return 0;
}

static int gm_lua_game_fill_rect(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int w = luaL_checkinteger(L, 4);
    int h = luaL_checkinteger(L, 5);
    int argc = lua_gettop(L);

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

    if (argc >= 8)
    {
        int r = luaL_checkinteger(L, 6);
        int g = luaL_checkinteger(L, 7);
        int b = luaL_checkinteger(L, 8);
        int a = 255;
        if (argc >= 9)
        {
            a = luaL_checkinteger(L, 9);
        }
        SDL_SetRenderDrawColor(game->renderer, gm_u8_clamp(r), gm_u8_clamp(g), gm_u8_clamp(b), gm_u8_clamp(a));
    }
    else
    {
        SDL_SetRenderDrawColor(game->renderer, game->r, game->g, game->b, game->a);
    }

    SDL_FRect rect = {
        .x = (float)x0,
        .y = (float)y0,
        .w = (float)(x1 - x0),
        .h = (float)(y1 - y0)};
    SDL_RenderFillRect(game->renderer, &rect);

    return 0;
}

static int gm_lua_game_noloop(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    game->stop_running = true;
    return 0;
}

static int gm_lua_game_save_pixels_to_image(lua_State *L)
{
    gm_lua_game_t *game = gm_lua_check_game(L);
    const char *filename = "frame.png";
    if (lua_gettop(L) >= 2)
    {
        filename = luaL_checkstring(L, 2);
    }

    SDL_Texture *prev_target = SDL_GetRenderTarget(game->renderer);
    SDL_SetRenderTarget(game->renderer, game->canvas_texture);
    SDL_Surface *surface = SDL_RenderReadPixels(game->renderer, NULL);
    SDL_SetRenderTarget(game->renderer, prev_target);

    if (!surface)
    {
        SDL_Log("Failed to read pixels: %s", SDL_GetError());
        return 0;
    }

    // Save to file
    if (SDL_SavePNG(surface, filename))
    {
        SDL_Log("Screenshot saved: %s", filename);
    }
    else
    {
        SDL_Log("Failed to save PNG: %s", SDL_GetError());
    }

    // Free the surface
    SDL_DestroySurface(surface);

    return 0;
}

int gm_lua_register_game_api(gm_lua_t *lua_ctx, SDL_Renderer *renderer, SDL_Texture *canvas_texture, int width, int height)
{
    lua_State *L = lua_ctx->L;

    luaL_newmetatable(L, GM_GAME_MT);

    lua_newtable(L);
    lua_pushcfunction(L, gm_lua_game_clear);
    lua_setfield(L, -2, "clear");
    lua_pushcfunction(L, gm_lua_game_noloop);
    lua_setfield(L, -2, "noLoop");
    lua_pushcfunction(L, gm_lua_game_set_color);
    lua_setfield(L, -2, "setColor");
    lua_pushcfunction(L, gm_lua_game_set_line_width);
    lua_setfield(L, -2, "setLineWidth");
    lua_pushcfunction(L, gm_lua_game_fill_rect);
    lua_setfield(L, -2, "fillRect");
    lua_pushcfunction(L, gm_lua_game_set_pixel);
    lua_setfield(L, -2, "setPixel");
    lua_pushcfunction(L, gm_lua_game_line);
    lua_setfield(L, -2, "line");
    lua_pushcfunction(L, gm_lua_game_save_pixels_to_image);
    lua_setfield(L, -2, "saveFrame");
    lua_pushinteger(L, width);
    lua_setfield(L, -2, "width");
    lua_pushinteger(L, height);
    lua_setfield(L, -2, "height");
    lua_setfield(L, -2, "__index");

    lua_pop(L, 1);

    gm_lua_game_t *gm = (gm_lua_game_t *)lua_newuserdata(L, sizeof(gm_lua_game_t));
    gm->renderer = renderer;
    gm->canvas_texture = canvas_texture;
    gm->w = width;
    gm->h = height;
    gm->r = 255;
    gm->g = 255;
    gm->b = 255;
    gm->a = 255;
    gm->line_width = 1;
    gm->stop_running = false;

    luaL_getmetatable(L, GM_GAME_MT);
    lua_setmetatable(L, -2);
    lua_setglobal(L, "gm");

    lua_ctx->gm = gm;

    return 0;
}
