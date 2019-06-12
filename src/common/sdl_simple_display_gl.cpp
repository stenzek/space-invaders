#include "sdl_simple_display_gl.h"
#include "YBaseLib/Assert.h"
#include "YBaseLib/Memory.h"
#include "YBaseLib/String.h"
#include <algorithm>

#pragma comment(lib, "opengl32.lib")

SDLSimpleDisplayGL::SDLSimpleDisplayGL() = default;

SDLSimpleDisplayGL::~SDLSimpleDisplayGL()
{
  if (m_framebuffer_texture != 0)
    glDeleteTextures(1, &m_framebuffer_texture);

  if (m_gl_context)
  {
    SDL_GL_MakeCurrent(nullptr, nullptr);
    SDL_GL_DeleteContext(m_gl_context);
  }
}

bool SDLSimpleDisplayGL::Initialize()
{
  if (!SDLSimpleDisplay::Initialize())
    return false;

  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  m_gl_context = SDL_GL_CreateContext(m_window);
  if (!m_gl_context)
  {
    Panic("Failed to create GL context");
    return nullptr;
  }

  SDL_GL_MakeCurrent(m_window, m_gl_context);

  // SDL_GL_SetSwapInterval(0);

  ResizeFramebuffer(m_framebuffer_width, m_framebuffer_height);
  return true;
}

u32 SDLSimpleDisplayGL::GetAdditionalWindowCreateFlags()
{
  return SDL_WINDOW_OPENGL;
}

void SDLSimpleDisplayGL::ResizeFramebuffer(uint32 width, uint32 height)
{
  if (m_framebuffer_texture != 0 && m_framebuffer_width == width && m_framebuffer_height == height)
    return;

  // Update buffer.
  m_framebuffer_width = width;
  m_framebuffer_height = height;
  m_framebuffer_data = std::vector<u32>(width * height);
  m_framebuffer_pointer = reinterpret_cast<byte*>(m_framebuffer_data.data());
  m_framebuffer_pitch = sizeof(u32) * width;

  // Update GL texture.
  if (m_framebuffer_texture == 0)
    glGenTextures(1, &m_framebuffer_texture);

  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_framebuffer_data.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void SDLSimpleDisplayGL::DisplayFramebuffer()
{
  s32 viewport_x, viewport_y;
  u32 viewport_width, viewport_height;
  CalculateDrawRectangle(&viewport_x, &viewport_y, &viewport_width, &viewport_height);

  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);

  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_framebuffer_width, m_framebuffer_height, GL_RGBA, GL_UNSIGNED_BYTE,
                  m_framebuffer_data.data());

  float texcoords[4][2] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, 0.0f}};
  for (u32 i = 0; i < 4; i++)
  {
    const float x = texcoords[i][0];
    const float y = texcoords[i][1];
    texcoords[i][0] = m_rotation_matrix[0][0] * x + m_rotation_matrix[0][1] * y;
    texcoords[i][1] = m_rotation_matrix[1][0] * x + m_rotation_matrix[1][1] * y;
  }

  glBegin(GL_QUADS);
  glTexCoord2fv(texcoords[0]);
  glVertex4f(-1.0f, 1.0f, 0.0f, 1.0f);
  glTexCoord2fv(texcoords[1]);
  glVertex4f(-1.0f, -1.0f, 0.0f, 1.0f);
  glTexCoord2fv(texcoords[2]);
  glVertex4f(1.0f, -1.0f, 0.0f, 1.0f);
  glTexCoord2fv(texcoords[3]);
  glVertex4f(1.0f, 1.0f, 0.0f, 1.0f);
  glEnd();

  SDL_GL_SwapWindow(m_window);
  AddFrameRendered();
}
