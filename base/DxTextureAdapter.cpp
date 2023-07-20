#include "DxTextureAdapter.h"

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID riid, void **ppFactory);

#include "string_utils.h"
#include "PixelFormatDefs.h"
#include <dxgi.h>
#include <vector>
#include <iostream>

int DxTextureAdapter::Init()
{
  std::cout << "DxTextureAdapter::Init" << std::endl;

  CreateDevice();
  if (!m_device)
  {
    std::cout << "null dx11 device.";
  }
  m_shareTextureHandle = 0;
  return 0;
}

int DxTextureAdapter::ResetDevice(ID3D11Device* pDevice)
{
  if (m_device.get() != pDevice)
  {
    *m_device.getForWrite() = pDevice;
     pDevice->GetImmediateContext(m_deviceContext.getForWrite());
    m_shareTextureHandle = 0;
    std::cout << "DxTextureAdapter::ResetDevice pDevice:" << std::hex << pDevice<< std::endl;
  }
  
  return 0;
}

int DxTextureAdapter::UnInit()
{
  m_shareTextureHandle = 0;
  m_shareTexture = nullptr;
  m_dxgiAdapter = nullptr;
  m_device = nullptr;
  m_deviceContext = nullptr;
  m_dxgiFactory = nullptr;
  m_stageTex = nullptr;

  std::cout << "DxTextureAdapter::UnInit"<< std::endl;;
  return 0;
}

int DxTextureAdapter::PrepareShareTexture(int64_t texture)
{
  if (m_device == nullptr)
  {
    return -2;
  }

  HANDLE hResource = (HANDLE)texture;
  if (hResource == (HANDLE)-1)
  {
    std::cout << "ProcessDxTexture EOF."<< std::endl;
    return -1;
  }

  if (m_shareTexture == nullptr || m_shareTextureHandle != texture)
  {
    m_shareTextureHandle = 0;
    m_shareTexture = nullptr;

    IDXGIResource* tempResource = NULL;
    auto ret = m_device->OpenSharedResource(hResource, __uuidof(IDXGIResource), (void**)& tempResource);
    if (!SUCCEEDED(ret))
    {
      std::cout << "OpenSharedResource failed, HANLDE: " << hResource
        << ", error code " << ret << std::endl;
      return -1;
    }

    tempResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&m_shareTexture);
    tempResource->Release();
    tempResource = nullptr;
    m_shareTextureHandle = texture;
    std::cout << "open new share texture:" << m_shareTexture<< std::endl;
  }

  ID3D11Texture2D* shareTexture = m_shareTexture.get();
  D3D11_TEXTURE2D_DESC srcTd;
  shareTexture->GetDesc(&srcTd);
  // FIXME(Von): DXGI_FORMAT_R8G8B8A8_UNORM_SRGB的颜色空间是sRGB,颜色(RGB)成分有个2.2的gamma校正,需要转换为线性空间
  if (srcTd.Format != DXGI_FORMAT_B8G8R8A8_UNORM &&
    srcTd.Format != DXGI_FORMAT_R8G8B8A8_UNORM &&
    srcTd.Format != DXGI_FORMAT_R8G8B8A8_UNORM_SRGB &&
    srcTd.Format != DXGI_FORMAT_R10G10B10A2_UNORM)
  {
    std::cout << "srcTd.Format is not DXGI_FORMAT_B8G8R8A8_UNORM or DXGI_FORMAT_R8G8B8A8_UNORM or DXGI_FORMAT_R10G10B10A2_UNORM, format:" 
    << srcTd.Format<< std::endl;
    shareTexture = nullptr;
    m_shareTexture = nullptr;
    m_shareTextureHandle = 0;
    return -1;
  }

  return 0;
}

int DxTextureAdapter::ProcessDxTexture(int64_t sharedTextureHandle, std::function<int(const D3D11_TEXTURE2D_DESC&, const D3D11_MAPPED_SUBRESOURCE&)> cb)
{
  if (PrepareShareTexture(sharedTextureHandle) != 0)
  {
    std::cout << "PrepareShareTexture failed, sharedTextureHandle = " << sharedTextureHandle;
    return -1;
  }

  ID3D11Texture2D* shareTexture = m_shareTexture.get();
  D3D11_TEXTURE2D_DESC srcTd;
  shareTexture->GetDesc(&srcTd);

  if (m_stageTex)
  {
    D3D11_TEXTURE2D_DESC td;
    m_stageTex->GetDesc(&td);
    if (td.Width != srcTd.Width || td.Height != srcTd.Height || td.Format != srcTd.Format)
    {
      m_stageTex = nullptr;
      CreateStageTex(srcTd.Width, srcTd.Height, srcTd.Format);
    }
  }
  else
  {
    CreateStageTex(srcTd.Width, srcTd.Height, srcTd.Format);
  }

  if (!m_device)
  {
    std::cout << "GetDevice = NULL" << std::endl;
    return -2;
  }
  m_device->GetImmediateContext(m_deviceContext.getForWrite());
  m_deviceContext->CopyResource(m_stageTex.get(), shareTexture);
  D3D11_MAPPED_SUBRESOURCE mapInfo;
  m_deviceContext->Map(m_stageTex.get(), 0, D3D11_MAP_READ, 0, &mapInfo);
  int ret = cb(srcTd, mapInfo);
  m_deviceContext->Unmap(m_stageTex.get(), 0);
  return ret;
}

int DxTextureAdapter::ProcessDxTexture(int64_t sharedTextureHandle, std::function<int(ID3D11Texture2D* pTexture)> cb)
{
  if (PrepareShareTexture(sharedTextureHandle) != 0)
  {
    std::cout << "PrepareShareTexture failed, sharedTextureHandle = " << sharedTextureHandle << std::endl;
    return -1;
  }

  ID3D11Texture2D* shareTexture = m_shareTexture.get();
  return cb(shareTexture);
}

int DxTextureAdapter::ProcessDxTexturePtr(ID3D11DeviceContext* pContext, ID3D11Resource* pResource, std::function<int(int64_t sharedTexture)> cb)
{
  if (!pResource)
  {
    std::cout << "pResource:" << std::hex << pResource << std::endl;
    return -11;
  }

  ID3D11Texture2D* pTexture = nullptr;
  HRESULT hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void **)&pTexture);
  if (FAILED(hr))
  {
    std::cout << "QueryInterface ID3D11Texture2D err:" << ::GetLastError() << std::endl;
    return -12;
  }

  ID3D11Device* pDevice = nullptr;
  pTexture->GetDevice(&pDevice);
  if (!pDevice)
  {
    std::cout << "GetDevice = NULL" << std::endl;
    return -13;
  }
  ID3D11DeviceContext *pImmediateContext = nullptr;
  pDevice->GetImmediateContext(&pImmediateContext);
  *m_device.getForWrite() = pDevice;
  *m_deviceContext.getForWrite() = pImmediateContext;

  D3D11_TEXTURE2D_DESC td;
  pTexture->GetDesc(&td);
  td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  td.Usage = D3D11_USAGE_DEFAULT;
  td.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

  if (td.Width != m_shareTextureDesc.Width || td.Height != m_shareTextureDesc.Height || td.Format != m_shareTextureDesc.Format)
  {
    std::cout << "D3D11_TEXTURE2D_DESC changed New DESC: width:" << td.Width << " height:" << td.Height << " format:" << td.Format
      << " Old DESC: width:" << m_shareTextureDesc.Width << " hegiht:" << m_shareTextureDesc.Height 
      << " format:" << m_shareTextureDesc.Format << std::endl;
    m_shareTexture = nullptr;
    pDevice->CreateTexture2D(&td, NULL, m_shareTexture.getForWrite());
    m_shareTextureDesc = td;
  }

  if (!m_shareTexture.get())
  {
    std::cout << "CreateTexture2D failed" << std::endl;
    return -14;
  }

  IDXGIResource* pDXGIResource = NULL;
  m_shareTexture->QueryInterface(__uuidof(IDXGIResource), (void**)&pDXGIResource);
  if (pDXGIResource)
  {
    pDXGIResource->GetSharedHandle((HANDLE*)&m_shareTextureHandle);
    pDXGIResource->Release();
  }

  pImmediateContext->CopyResource(m_shareTexture.get(), pTexture);
  pTexture->Release();
  return cb(m_shareTextureHandle);
}

bool DxTextureAdapter::CreateDevice()
{
  HMODULE hD3D11 = ::LoadLibrary(L"D3D11.dll");
  if (!hD3D11)
  {
    std::cout << "LoadLibrary D3D11.dll failed" << std::endl;
    return false;
  }

  HMODULE hDXGI = ::LoadLibrary(L"dxgi.dll");
  if (!hDXGI)
  {
    std::cout << "LoadLibrary dxgi.dll failed" << std::endl;
    return false;
  }

  auto* pfnD3D11CreateDevice =
    (PFN_D3D11_CREATE_DEVICE)::GetProcAddress(hD3D11, "D3D11CreateDevice");
  if (!pfnD3D11CreateDevice)
  {
    std::cout << "GetProcAddress D3D11CreateDevice failed" << std::endl;
    return false;
  }

  auto* createDXGIFactory1 =
    (CREATEDXGIFACTORY1PROC)::GetProcAddress(hDXGI, "CreateDXGIFactory1");
  if (!createDXGIFactory1)
  {
    std::cout << " can't found CreateDXGIFactory1" << std::endl;
    return false;
  }

  HRESULT hr = S_OK;
  rtc::scoped_refptr<IDXGIFactory1> pFactory;
  if (FAILED(hr = (*createDXGIFactory1)(__uuidof(IDXGIFactory1), (void**)&pFactory)))
  {
    std::cout << " create dxgifactory1 failed. error code " << hr << std::endl;
    return false;
  }

  rtc::scoped_refptr<IDXGIAdapter1> pCurrentAdapter;
  std::vector<rtc::scoped_refptr<IDXGIAdapter1>> vAdapters;
  int index = 0;
  while (pFactory->EnumAdapters1(index, pCurrentAdapter.getForWrite()) != DXGI_ERROR_NOT_FOUND)
  {
    DXGI_ADAPTER_DESC1 desc;
    pCurrentAdapter->GetDesc1(&desc);

    if (desc.VendorId != 0x1414)  // Basic render
    {
      vAdapters.push_back(pCurrentAdapter);
    }

    pCurrentAdapter = nullptr;
    ++index;
  }

  DWORD videocardId = 0;
  std::string videoCardName;
  if (vAdapters.size() > 1)
  {
    for (auto& vAdapter : vAdapters)
    {
      pCurrentAdapter = vAdapter;
      DXGI_ADAPTER_DESC1 desc;
      pCurrentAdapter->GetDesc1(&desc);
      videoCardName = rtc::ToUtf8(desc.Description);
      videocardId = desc.VendorId;

      if (desc.VendorId != 0x8086)  // intel graphic
      {
        break;
      }
    }
  }
  else
  {
    if (vAdapters.empty())
    {
      return false;
    }

    pCurrentAdapter = vAdapters[0];
    DXGI_ADAPTER_DESC1 desc;
    pCurrentAdapter->GetDesc1(&desc);
    videoCardName = rtc::ToUtf8(desc.Description);
    videocardId = desc.VendorId;
  }
  std::cout << " use video card " << videoCardName.c_str()
    << ", vendor id:" << videocardId << std::endl;

  if (!pCurrentAdapter)
  {
    return false;
  }

  constexpr D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

  DWORD dwFlags = 0;
  UINT SDKVersion = D3D11_SDK_VERSION;
  D3D_FEATURE_LEVEL outLevel;
  hr = pfnD3D11CreateDevice(pCurrentAdapter.get(), D3D_DRIVER_TYPE_UNKNOWN, NULL,
    dwFlags, featureLevels, ARRAYSIZE(featureLevels),
    SDKVersion, m_device.getForWrite(), &outLevel, m_deviceContext.getForWrite());
  if (FAILED(hr))
  {
    std::cout << "D3D11CreateDevice by Specified Adapter with D3D_DRIVER_TYPE_UNKNOWN failed, err:" << std::hex << hr << std::endl;
    hr = pfnD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, dwFlags,
      featureLevels, ARRAYSIZE(featureLevels),
      SDKVersion, m_device.getForWrite(), &outLevel,
      m_deviceContext.getForWrite());

    if (FAILED(hr))
    {
      std::cout << "D3D11CreateDevice by NULL Adapter with D3D_DRIVER_TYPE_WARP failed, err:" << std::hex << hr << std::endl;
      return false;
    }
  }

  std::cout << " create d3d11 device success.";
  return true;
}

void DxTextureAdapter::CreateStageTex(int width, int height, int format)
{
  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = width;
  desc.Height = height;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = (DXGI_FORMAT)format;

  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.MiscFlags = 0;
  desc.BindFlags = 0;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  HRESULT hr = m_device->CreateTexture2D(&desc, NULL, m_stageTex.getForWrite());
  std::cout << "create new stage texture " <<
    (FAILED(hr) ? "failed" : "succeed") << " with width = " << width
    << ", height = " << height << ", pixelFormat = " << format << std::endl;
}


