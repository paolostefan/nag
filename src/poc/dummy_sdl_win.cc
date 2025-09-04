#include <SDL.h>
#include <iostream>

/**
 * @file dummy_sdl_win.cc
 * @brief A trivial SDL2 window.
 */

int main(int argc, char **argv)
{
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS) != 0)
  {
    std::cerr << "Error initializing SDL: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow(
      "My Demo Engine",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      800, 600,
      SDL_WINDOW_SHOWN);

  if (!window)
  {
    std::cerr << "Error creating window: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!renderer)
  {
    std::cerr << "Error creating renderer: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  bool running = true;
  SDL_Event event;

  while (running)
  {
    while (SDL_PollEvent(&event))
    {
      if (event.type == SDL_QUIT)
      {
        running = false;
      }
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
      {
        running = false;
      }
    }

    // Clear screen
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    // Draw some stuff...
    SDL_SetRenderDrawColor(renderer, 200, 50, 50, 255);
    SDL_Rect rect = {350, 250, 100, 100};
    SDL_RenderFillRect(renderer, &rect);

    // Present frame
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}
