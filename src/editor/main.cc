#include <string>

#include "SDL.h"
#include "spdlog/spdlog.h"

#include "editor.h"

int main(int argc, char **argv)
{
  // Init SDL with OpenGL support
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0)
  {
    spdlog::critical("Error initializing SDL: {}", SDL_GetError());
    return -1;
  }

  return Editor().run();
}
