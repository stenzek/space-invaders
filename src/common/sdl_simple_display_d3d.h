#pragma once
#include "YBaseLib/Common.h"

#ifdef Y_PLATFORM_WINDOWS

#include "YBaseLib/Windows/WindowsHeaders.h"
#include "sdl_simple_display.h"
#include <d3d11.h>
#include <memory>
#include <wrl.h>

class SDLSimpleDisplayD3D final : public SDLSimpleDisplay
{
public:
  SDLSimpleDisplayD3D();
  ~SDLSimpleDisplayD3D();

  void ResizeFramebuffer(u32 width, u32 height) override;
  void DisplayFramebuffer() override;

protected:
  bool Initialize() override;
  void OnWindowResized() override;

private:
  bool CreateRenderTargetView();

  Microsoft::WRL::ComPtr<ID3D11Device> m_device = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> m_context = nullptr;
  Microsoft::WRL::ComPtr<IDXGISwapChain> m_swap_chain = nullptr;
  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_swap_chain_rtv = nullptr;
  Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertex_shader = nullptr;
  Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixel_shader = nullptr;
  Microsoft::WRL::ComPtr<ID3D11RasterizerState> m_rasterizer_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depth_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11BlendState> m_blend_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11SamplerState> m_sampler_state = nullptr;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> m_framebuffer_texture = nullptr;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_framebuffer_texture_srv = nullptr;

  bool m_framebuffer_texture_mapped = false;
};

#endif