#pragma once

#include <memory>
#include <functional>
#include "scoped_refptr.h"
#include <d3d11.h>


struct IDXGIAdapter;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGIFactory;
struct ID3D11Texture2D;
struct D3D11_MAPPED_SUBRESOURCE;
struct D3D11_TEXTURE2D_DESC;

////////////////////////////////////////////////////////////////////////
// DxTexture 
class DxTextureAdapter
{
public:
  int Init();
  int ResetDevice(ID3D11Device* pDevice);
  int UnInit();

  int ProcessDxTexture(int64_t sharedTextureHandle, std::function<int(const D3D11_TEXTURE2D_DESC&, const D3D11_MAPPED_SUBRESOURCE&)> cb);
  int ProcessDxTexture(int64_t sharedTextureHandle, std::function<int(ID3D11Texture2D* pTexture)> cb);
  int ProcessDxTexturePtr(ID3D11DeviceContext* pContext, ID3D11Resource* pResource, std::function<int(int64_t sharedTexture)> cb);

  ID3D11Device* GetDevice() { return m_device.get(); }
  ID3D11DeviceContext* GetDeviceContext() { return m_deviceContext.get(); }
private:
  void CreateStageTex(int width, int height, int format);
  bool CreateDevice();
  int  PrepareShareTexture(int64_t texture);
private:
  rtc::scoped_refptr<IDXGIAdapter> m_dxgiAdapter;
  rtc::scoped_refptr<ID3D11Device> m_device;
  rtc::scoped_refptr<ID3D11DeviceContext> m_deviceContext;
  rtc::scoped_refptr<IDXGIFactory> m_dxgiFactory;
  rtc::scoped_refptr<ID3D11Texture2D> m_stageTex;

  rtc::scoped_refptr<ID3D11Texture2D> m_shareTexture;
  D3D11_TEXTURE2D_DESC m_shareTextureDesc;
  uint64_t m_shareTextureHandle = 0;
};