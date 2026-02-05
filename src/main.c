// minimal SDL3 program
#include <stdio.h>
#include <SDL3/SDL.h>

int main(int argc, char *argv[])
{
    // dimensions of the window
    const int SCREEN_WIDTH = 800;
    const int SCREEN_HEIGHT = 600;

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

    // application/game loop
    int quit = 0;
    SDL_Event evt;
    while (quit == 0)
    {
        // update
        // draw
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
