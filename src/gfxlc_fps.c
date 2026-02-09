#include <stdlib.h>
#include <stdio.h>
#include "gfxlc_fps.h"

int gfxlc_fps_init(gfxlc_fps_t **fps)
{
    (*fps) = (gfxlc_fps_t *)calloc(sizeof(gfxlc_fps_t), 1);
    if ((*fps) == NULL)
    {
        printf("Unable to allocate memory for gfxlc_fps_t.\n");
        return 1;
    }

    gfxlc_fps_t *f = (*fps);
    f->frameCount = 0;
    f->lastUpdateTime = 0;
    f->fpsTexture = NULL;
    f->currentFPS = 0.0f;

    return 0;
}

void gfxlc_fps_draw(gfxlc_fps_t *fps, SDL_Renderer *renderer, TTF_Font *font, int x, int y)
{
    uint64_t currentTime = SDL_GetTicks();
    fps->frameCount++;
    fps->fpsRect.x = (float)x;
    fps->fpsRect.y = (float)y;

    if (currentTime > fps->lastUpdateTime + 500)
    {
        float elapsedSeconds = (currentTime - fps->lastUpdateTime) / 1000.0f;
        fps->currentFPS = (elapsedSeconds > 0.0f) ? (fps->frameCount / elapsedSeconds) : 0.0f;
        fps->frameCount = 0;
        fps->lastUpdateTime = currentTime;

        if (fps->fpsTexture)
        {
            SDL_DestroyTexture(fps->fpsTexture);
        }

        char text[32];
        snprintf(text, sizeof(text), "FPS: %.2f", fps->currentFPS);
        SDL_Color fg = {255, 255, 255, 255};
        SDL_Surface *surf = TTF_RenderText_Blended(font, text, 0, fg);
        if (surf)
        {
            fps->fpsTexture = SDL_CreateTextureFromSurface(renderer, surf);
            // Set scale mode to NEAREST to get sharp pixel edges
            SDL_SetTextureScaleMode(fps->fpsTexture, SDL_SCALEMODE_PIXELART);

            fps->fpsRect.w = (float)surf->w;
            fps->fpsRect.h = (float)surf->h;
            SDL_DestroySurface(surf);
        }
    }

    if (fps->fpsTexture)
    {
        SDL_RenderTexture(renderer, fps->fpsTexture, NULL, &fps->fpsRect);
    }
}

void gfxlc_fps_shutdown(gfxlc_fps_t *fps)
{
    if (fps)
    {
        if (fps->fpsTexture)
        {
            SDL_DestroyTexture(fps->fpsTexture);
        }
        free(fps);
    }
}
