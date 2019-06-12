#include "YBaseLib/Log.h"
#include "common/sdl_simple_display.h"
#include "i8080/cpu.h"
#include "system.h"
#include <SDL.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <memory>
#include <optional>
#include <vector>
Log_SetChannel(Invaders);

static void HandleKeyEvent(const SDL_Event* ev, Invaders::Inputs& inputs)
{
  const bool down = (ev->type == SDL_KEYDOWN);
  switch (ev->key.keysym.sym)
  {
    case SDLK_a:
    case SDLK_LEFT:
      inputs.left_1p = down;
      break;
    case SDLK_d:
    case SDLK_RIGHT:
      inputs.right_1p = down;
      break;
    case SDLK_w:
    case SDLK_UP:
    case SDLK_SPACE:
      inputs.fire_1p = down;
      break;
    case SDLK_KP_4:
      inputs.left_2p = down;
      break;
    case SDLK_KP_6:
      inputs.right_2p = down;
      break;
    case SDLK_KP_8:
    case SDLK_KP_ENTER:
      inputs.fire_2p = down;
      break;

    case SDLK_RETURN:
      inputs.credit = down;
      break;
    case SDLK_1:
      inputs.start_1p = down;
      break;
    case SDLK_2:
      inputs.start_2p = down;
      break;
  }
}

int main(int argc, char* argv[])
{
  Log::GetInstance().SetConsoleOutputParams(true);

  // i8080::TRACE_EXECUTION = true;

  auto system = std::make_unique<Invaders::System>();
  if (!system->LoadROMs("invaders"))
  {
    Log_ErrorPrintf("Failed to load ROMs.");
    return EXIT_FAILURE;
  }

  auto display = SDLSimpleDisplay::Create();
  if (!display)
  {
    Log_ErrorPrintf("Failed to create display");
    return EXIT_FAILURE;
  }

  if (!system->Initialize(display.get()))
  {
    Log_ErrorPrintf("Failed to initialize system");
    return EXIT_FAILURE;
  }

  bool running = true;
  while (running)
  {
    // SDL event loop...
    for (;;)
    {
      SDL_Event ev;
      if (!SDL_PollEvent(&ev))
        break;

      if (display->HandleSDLEvent(&ev))
        continue;

      switch (ev.type)
      {
        case SDL_KEYDOWN:
        case SDL_KEYUP:
        {
          HandleKeyEvent(&ev, system->GetInputs());
          if (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_PAUSE)
            system->Reset();
        }
        break;

        case SDL_QUIT:
          running = false;
          break;
      }
    }

    system->ExecuteFrame();
  }

  return 0;
}