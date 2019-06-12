#pragma once
#include "YBaseLib/Timer.h"
#include "types.h"
#include <memory>

class SimpleDisplay
{
public:
  SimpleDisplay();
  virtual ~SimpleDisplay();

  u32 GetFramesRendered() const { return m_frames_rendered; }
  float GetFramesPerSecond() const { return m_fps; }
  void ResetFramesRendered() { m_frames_rendered = 0; }

  u32 GetDisplayWidth() const { return m_display_width; }
  u32 GetDisplayHeight() const { return m_display_height; }
  void SetDisplayScale(u32 scale) { m_display_scale = scale; }
  void SetDisplayAspectRatio(u32 numerator, u32 denominator);
  virtual void ResizeDisplay(u32 width = 0, u32 height = 0);

  u32 GetFramebufferWidth() const { return m_framebuffer_width; }
  u32 GetFramebufferHeight() const { return m_framebuffer_height; }
  byte* GetFramebufferPointer() const { return m_framebuffer_pointer; }
  u32 GetFramebufferPitch() const { return m_framebuffer_pitch; }
  void ClearFramebuffer();
  virtual void ResizeFramebuffer(u32 width, u32 height) = 0;
  virtual void DisplayFramebuffer() = 0;

  static constexpr u32 PackRGB(u8 r, u8 g, u8 b)
  {
    return (static_cast<u32>(r) << 0) | (static_cast<u32>(g) << 8) | (static_cast<u32>(b) << 16) |
           (static_cast<u32>(0xFF) << 24);
  }

  void SetPixel(u32 x, u32 y, u8 r, u8 g, u8 b);
  void SetPixel(u32 x, u32 y, u32 rgb);
  void CopyFrame(const void* pixels, u32 stride);

  void SetRotation(float degrees);

protected:
  void AddFrameRendered();
  void CalculateDrawRectangle(s32* x, s32* y, u32* width, u32* height);

  u32 m_framebuffer_width = 640;
  u32 m_framebuffer_height = 480;
  byte* m_framebuffer_pointer = nullptr;
  u32 m_framebuffer_pitch = 0;

  u32 m_display_width = 640;
  u32 m_display_height = 480;
  u32 m_display_scale = 1;
  u32 m_display_aspect_numerator = 1;
  u32 m_display_aspect_denominator = 1;

  static const u32 FRAME_COUNTER_FRAME_COUNT = 100;
  Timer m_frame_counter_timer;
  u32 m_frames_rendered = 0;
  float m_fps = 0.0f;

  float m_rotation_matrix[2][2] = { 1.0f, 0.0f, 0.0f, 1.0f };
};

class NullDisplay : public SimpleDisplay
{
public:
  NullDisplay();
  ~NullDisplay();

  static std::unique_ptr<SimpleDisplay> Create();

  void ResizeFramebuffer(u32 width, u32 height) override;
  void DisplayFramebuffer() override;
};