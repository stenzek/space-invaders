#pragma once
#include "common/simple_display.h"
#include "common/types.h"
#include <memory>
#include <vector>

union SDL_Event;
struct SDL_Window;

class SDLSimpleDisplay : public SimpleDisplay
{
public:
  SDLSimpleDisplay();
  ~SDLSimpleDisplay();

  static std::unique_ptr<SDLSimpleDisplay> Create();

  virtual void ResizeDisplay(u32 width = 0, u32 height = 0) override;

  virtual bool HandleSDLEvent(const SDL_Event* ev);

  SDL_Window* GetSDLWindow() const { return m_window; }

  bool IsFullscreen() const;
  void SetFullscreen(bool enable);

protected:
  virtual u32 GetAdditionalWindowCreateFlags() { return 0; }
  virtual bool Initialize();
  virtual void OnWindowResized();

  SDL_Window* m_window = nullptr;
};

