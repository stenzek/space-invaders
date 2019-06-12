#include "sdl_simple_display_d3d.h"
#ifdef Y_PLATFORM_WINDOWS
#include "YBaseLib/Assert.h"
#include <SDL_syswm.h>
#include <algorithm>

#pragma comment(lib, "d3d11.lib")

static const uint32 SWAP_CHAIN_BUFFER_COUNT = 2;
static const DXGI_FORMAT SWAP_CHAIN_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;

static const uint32 VS_BYTECODE[] = {
  0x43425844, 0x0608a44a, 0xd0e1754a, 0xec57c233, 0x42017a39, 0x00000001, 0x000002a8, 0x00000005, 0x00000034,
  0x00000080, 0x000000b4, 0x0000010c, 0x0000022c, 0x46454452, 0x00000044, 0x00000000, 0x00000000, 0x00000000,
  0x0000001c, 0xfffe0400, 0x00000100, 0x0000001c, 0x7263694d, 0x666f736f, 0x52282074, 0x4c482029, 0x53204c53,
  0x65646168, 0x6f432072, 0x6c69706d, 0x31207265, 0x00312e30, 0x4e475349, 0x0000002c, 0x00000001, 0x00000008,
  0x00000020, 0x00000000, 0x00000006, 0x00000001, 0x00000000, 0x00000101, 0x565f5653, 0x65747265, 0x00444978,
  0x4e47534f, 0x00000050, 0x00000002, 0x00000008, 0x00000038, 0x00000000, 0x00000000, 0x00000003, 0x00000000,
  0x00000c03, 0x00000041, 0x00000000, 0x00000001, 0x00000003, 0x00000001, 0x0000000f, 0x43584554, 0x44524f4f,
  0x5f565300, 0x69736f50, 0x6e6f6974, 0xababab00, 0x52444853, 0x00000118, 0x00010040, 0x00000046, 0x04000060,
  0x00101012, 0x00000000, 0x00000006, 0x03000065, 0x00102032, 0x00000000, 0x04000067, 0x001020f2, 0x00000001,
  0x00000001, 0x02000068, 0x00000001, 0x07000029, 0x00100012, 0x00000000, 0x0010100a, 0x00000000, 0x00004001,
  0x00000001, 0x07000001, 0x00100012, 0x00000000, 0x0010000a, 0x00000000, 0x00004001, 0x00000002, 0x07000001,
  0x00100042, 0x00000000, 0x0010100a, 0x00000000, 0x00004001, 0x00000002, 0x05000056, 0x00100032, 0x00000000,
  0x00100086, 0x00000000, 0x05000036, 0x00102032, 0x00000000, 0x00100046, 0x00000000, 0x0f000032, 0x00102032,
  0x00000001, 0x00100046, 0x00000000, 0x00004002, 0x40000000, 0xc0000000, 0x00000000, 0x00000000, 0x00004002,
  0xbf800000, 0x3f800000, 0x00000000, 0x00000000, 0x08000036, 0x001020c2, 0x00000001, 0x00004002, 0x00000000,
  0x00000000, 0x00000000, 0x3f800000, 0x0100003e, 0x54415453, 0x00000074, 0x00000008, 0x00000001, 0x00000000,
  0x00000003, 0x00000001, 0x00000001, 0x00000002, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000000,
  0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

static const uint32 PS_BYTECODE[] = {
  0x43425844, 0x76fb5edf, 0x6680f045, 0x1a81341f, 0xac4335f9, 0x00000001, 0x0000021c, 0x00000005, 0x00000034,
  0x000000cc, 0x00000100, 0x00000134, 0x000001a0, 0x46454452, 0x00000090, 0x00000000, 0x00000000, 0x00000002,
  0x0000001c, 0xffff0400, 0x00000100, 0x00000067, 0x0000005c, 0x00000003, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000001, 0x00000000, 0x00000062, 0x00000002, 0x00000005, 0x00000004, 0xffffffff, 0x00000000,
  0x00000001, 0x0000000c, 0x706d6173, 0x65740030, 0x4d003078, 0x6f726369, 0x74666f73, 0x29522820, 0x534c4820,
  0x6853204c, 0x72656461, 0x6d6f4320, 0x656c6970, 0x30312072, 0xab00312e, 0x4e475349, 0x0000002c, 0x00000001,
  0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003, 0x00000000, 0x00000303, 0x43584554, 0x44524f4f,
  0xababab00, 0x4e47534f, 0x0000002c, 0x00000001, 0x00000008, 0x00000020, 0x00000000, 0x00000000, 0x00000003,
  0x00000000, 0x0000000f, 0x545f5653, 0x65677261, 0xabab0074, 0x52444853, 0x00000064, 0x00000040, 0x00000019,
  0x0300005a, 0x00106000, 0x00000000, 0x04001858, 0x00107000, 0x00000000, 0x00005555, 0x03001062, 0x00101032,
  0x00000000, 0x03000065, 0x001020f2, 0x00000000, 0x09000045, 0x001020f2, 0x00000000, 0x00101046, 0x00000000,
  0x00107e46, 0x00000000, 0x00106000, 0x00000000, 0x0100003e, 0x54415453, 0x00000074, 0x00000002, 0x00000000,
  0x00000000, 0x00000002, 0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
  0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000};

SDLSimpleDisplayD3D::SDLSimpleDisplayD3D() = default;

SDLSimpleDisplayD3D::~SDLSimpleDisplayD3D()
{
  if (m_framebuffer_texture_mapped)
    m_context->Unmap(m_framebuffer_texture.Get(), 0);
}

bool SDLSimpleDisplayD3D::Initialize()
{
  if (!SDLSimpleDisplay::Initialize())
    return false;

  SDL_SysWMinfo info = {};
  SDL_VERSION(&info.version);
  if (!SDL_GetWindowWMInfo(m_window, &info))
    return nullptr;

  DXGI_SWAP_CHAIN_DESC desc = {};
  desc.BufferDesc.Format = SWAP_CHAIN_BUFFER_FORMAT;
  desc.BufferUsage = DXGI_USAGE_BACK_BUFFER | DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
  desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Windowed = TRUE;
  desc.OutputWindow = info.info.win.window;

  D3D_FEATURE_LEVEL feature_level;
  HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, nullptr, 0, D3D11_SDK_VERSION,
                                             &desc, m_swap_chain.GetAddressOf(), m_device.GetAddressOf(),
                                             &feature_level, m_context.GetAddressOf());
  if (FAILED(hr) || feature_level < D3D_FEATURE_LEVEL_10_0)
    return false;

  // Disable DXGI responding to ALT+ENTER, we need to capture these keystrokes and handle it ourselves.
  Microsoft::WRL::ComPtr<IDXGIFactory> dxgi_factory;
  hr = m_swap_chain->GetParent(IID_PPV_ARGS(dxgi_factory.GetAddressOf()));
  if (FAILED(hr))
    return false;

  hr = dxgi_factory->MakeWindowAssociation(desc.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER |
                                                                DXGI_MWA_NO_PRINT_SCREEN);
  if (FAILED(hr))
    return false;

  if (!CreateRenderTargetView())
    return false;

  hr = m_device->CreateVertexShader(VS_BYTECODE, sizeof(VS_BYTECODE), nullptr, m_vertex_shader.GetAddressOf());
  if (FAILED(hr))
    return false;

  hr = m_device->CreatePixelShader(PS_BYTECODE, sizeof(PS_BYTECODE), nullptr, m_pixel_shader.GetAddressOf());
  if (FAILED(hr))
    return false;

  D3D11_RASTERIZER_DESC rs_desc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
  hr = m_device->CreateRasterizerState(&rs_desc, m_rasterizer_state.GetAddressOf());
  if (FAILED(hr))
    return false;

  D3D11_DEPTH_STENCIL_DESC ds_desc = CD3D11_DEPTH_STENCIL_DESC(CD3D11_DEFAULT());
  ds_desc.DepthEnable = FALSE;
  ds_desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
  hr = m_device->CreateDepthStencilState(&ds_desc, m_depth_state.GetAddressOf());
  if (FAILED(hr))
    return false;

  D3D11_BLEND_DESC bs_desc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
  hr = m_device->CreateBlendState(&bs_desc, m_blend_state.GetAddressOf());
  if (FAILED(hr))
    return false;

  D3D11_SAMPLER_DESC ss_desc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
  // ss_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  ss_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  hr = m_device->CreateSamplerState(&ss_desc, m_sampler_state.GetAddressOf());
  if (FAILED(hr))
    return false;

  ResizeFramebuffer(m_framebuffer_width, m_framebuffer_height);
  if (!m_framebuffer_texture_mapped)
    return false;

  return true;
}

bool SDLSimpleDisplayD3D::CreateRenderTargetView()
{
  D3D11_RENDER_TARGET_VIEW_DESC desc = {};
  desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
  desc.Format = SWAP_CHAIN_BUFFER_FORMAT;
  desc.Texture2D.MipSlice = 0;

  ID3D11Texture2D* surface;
  HRESULT hr = m_swap_chain->GetBuffer(0, IID_PPV_ARGS(&surface));
  if (FAILED(hr))
    return false;

  hr = m_device->CreateRenderTargetView(surface, &desc, m_swap_chain_rtv.GetAddressOf());
  surface->Release();
  return SUCCEEDED(hr);
}

void SDLSimpleDisplayD3D::ResizeFramebuffer(u32 width, u32 height)
{
  if (m_framebuffer_texture && m_framebuffer_width == width && m_framebuffer_height == height)
    return;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> framebuffer_texture;
  Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> framebuffer_texture_srv;

  D3D11_TEXTURE2D_DESC desc =
    CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE,
                          D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, framebuffer_texture.ReleaseAndGetAddressOf());
  if (FAILED(hr))
  {
    Panic("Failed to create framebuffer texture.");
    return;
  }

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc =
    CD3D11_SHADER_RESOURCE_VIEW_DESC(framebuffer_texture.Get(), D3D11_SRV_DIMENSION_TEXTURE2D);
  hr = m_device->CreateShaderResourceView(framebuffer_texture.Get(), &srv_desc,
                                          framebuffer_texture_srv.ReleaseAndGetAddressOf());
  if (FAILED(hr))
  {
    Panic("Failed to create framebuffer texture SRV.");
    return;
  }

  D3D11_MAPPED_SUBRESOURCE sr;
  hr = m_context->Map(framebuffer_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);
  if (FAILED(hr))
  {
    Panic("Failed to map framebuffer texture.");
    return;
  }

  // Unmap old framebuffer before releasing it.
  if (m_framebuffer_texture_mapped)
  {
    m_context->Unmap(m_framebuffer_texture.Get(), 0);
    m_framebuffer_texture_mapped = false;
  }

  m_framebuffer_width = width;
  m_framebuffer_height = height;
  m_framebuffer_texture = std::move(framebuffer_texture);
  m_framebuffer_texture_srv = std::move(framebuffer_texture_srv);
  m_framebuffer_pointer = reinterpret_cast<byte*>(sr.pData);
  m_framebuffer_pitch = sr.RowPitch;
  m_framebuffer_texture_mapped = true;
}

void SDLSimpleDisplayD3D::DisplayFramebuffer()
{
  // Unmap framebuffer before displaying.
  if (m_framebuffer_texture_mapped)
  {
    m_context->Unmap(m_framebuffer_texture.Get(), 0);
    m_framebuffer_texture_mapped = false;
  }

  s32 viewport_x, viewport_y;
  u32 viewport_width, viewport_height;
  CalculateDrawRectangle(&viewport_x, &viewport_y, &viewport_width, &viewport_height);

  // Handle GL bottom-left origin.
  viewport_y = s32(m_display_height) - viewport_y - s32(viewport_height);

  static const float clear_color[] = {0.0f, 0.0f, 0.0f, 1.0f};
  m_context->ClearRenderTargetView(m_swap_chain_rtv.Get(), clear_color);

  m_context->OMSetRenderTargets(1, m_swap_chain_rtv.GetAddressOf(), nullptr);
  m_context->RSSetState(m_rasterizer_state.Get());
  m_context->OMSetDepthStencilState(m_depth_state.Get(), 0);
  m_context->OMSetBlendState(m_blend_state.Get(), nullptr, 0xFFFFFFFF);

  D3D11_VIEWPORT vp =
    CD3D11_VIEWPORT(float(viewport_x), float(viewport_y), float(viewport_width), float(viewport_height));
  m_context->RSSetViewports(1, &vp);

  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_context->VSSetShader(m_vertex_shader.Get(), nullptr, 0);
  m_context->PSSetShader(m_pixel_shader.Get(), nullptr, 0);
  m_context->PSSetShaderResources(0, 1, m_framebuffer_texture_srv.GetAddressOf());
  m_context->PSSetSamplers(0, 1, m_sampler_state.GetAddressOf());

  m_context->Draw(3, 0);

  // m_swap_chain->Present(0, 0);
  m_swap_chain->Present(1, 0);

  // Re-map framebuffer texture.
  for (;;)
  {
    D3D11_MAPPED_SUBRESOURCE sr;
    HRESULT hr = m_context->Map(m_framebuffer_texture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &sr);
    if (FAILED(hr))
    {
      Panic("Failed to map framebuffer texture.");
      continue;
    }

    m_framebuffer_pointer = reinterpret_cast<byte*>(sr.pData);
    m_framebuffer_pitch = sr.RowPitch;
    m_framebuffer_texture_mapped = true;
    break;
  }

  AddFrameRendered();
}

void SDLSimpleDisplayD3D::OnWindowResized()
{
  SDLSimpleDisplay::OnWindowResized();

  m_context->OMSetRenderTargets(0, nullptr, nullptr);
  m_swap_chain_rtv.Reset();

  HRESULT hr = m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, m_display_width, m_display_height,
                                           SWAP_CHAIN_BUFFER_FORMAT, 0);
  if (FAILED(hr) || !CreateRenderTargetView())
    Panic("Failed to resize swap chain buffers.");
}

#endif