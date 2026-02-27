#include "pch.h"
#include "WGCWindowCapture.h"
#include <sstream>

namespace
{
    std::string HResultToString(HRESULT hr)
    {
        std::stringstream ss;
        ss << "0x" << std::hex << hr;
        return ss.str();
    }
}

WGCWindowCapture::WGCWindowCapture()
    : m_initialized(false)
{
}

WGCWindowCapture::~WGCWindowCapture()
{
    StopContinuousCapture();
    Cleanup();
}

bool WGCWindowCapture::Initialize(std::string* outError)
{
    auto setError = [&](const std::string& msg) {
        if (outError) *outError = msg;
    };

    if (m_initialized)
    {
        return true;
    }

    try
    {
        auto d3dDevice = util::CreateD3D11Device();
        if (!d3dDevice)
        {
            setError("Failed to create D3D11 device");
            return false;
        }

        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        if (!dxgiDevice)
        {
            setError("Failed to get DXGI device");
            return false;
        }

        m_device = CreateDirect3DDevice(dxgiDevice.get());
        if (!m_device)
        {
            setError("Failed to create Direct3D device");
            return false;
        }

        m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
        if (!m_d3dDevice)
        {
            setError("Failed to get D3D11 device interface");
            return false;
        }

        m_d3dDevice->GetImmediateContext(m_d3dContext.put());
        if (!m_d3dContext)
        {
            setError("Failed to get device context");
            return false;
        }

        m_initialized = true;
        return true;
    }
    catch (const winrt::hresult_error& e)
    {
        std::stringstream ss;
        ss << "WinRT error: " << winrt::to_string(e.message()) 
           << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    }
    catch (...)
    {
        setError("Unknown exception during initialization");
        return false;
    }
}

void WGCWindowCapture::Cleanup()
{
    if (!m_initialized) return;

    m_d3dContext = nullptr;
    m_d3dDevice = nullptr;
    m_device = nullptr;
    m_initialized = false;
}

bool WGCWindowCapture::CreateTextures(UINT width, UINT height)
{
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    for (int i = 0; i < 2; i++)
    {
        HRESULT hr = m_d3dDevice->CreateTexture2D(&desc, nullptr, m_stagingTextures[i].put());
        if (FAILED(hr)) return false;
    }

    m_textureWidth = width;
    m_textureHeight = height;
    m_currentStagingIndex = 0;
    m_readableStagingIndex = -1;
    
    return true;
}

bool WGCWindowCapture::StartContinuousCapture(HWND hwnd, std::string* outError)
{
    auto setError = [&](const std::string& msg) {
        if (outError) *outError = msg;
    };

    if (!m_initialized)
    {
        setError("Not initialized");
        return false;
    }

    if (m_isCapturing)
    {
        StopContinuousCapture();
    }

    try
    {
        m_captureItem = util::CreateCaptureItemForWindow(hwnd);
        if (!m_captureItem)
        {
            setError("Failed to create capture item");
            return false;
        }

        auto size = m_captureItem.Size();
        if (size.Width <= 0 || size.Height <= 0)
        {
            setError("Invalid window size");
            return false;
        }

        if (!CreateTextures(size.Width, size.Height))
        {
            setError("Failed to create textures");
            return false;
        }

        m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
            m_device,
            winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            size);
        
        if (!m_framePool)
        {
            setError("Failed to create frame pool");
            return false;
        }

        m_session = m_framePool.CreateCaptureSession(m_captureItem);
        if (!m_session)
        {
            setError("Failed to create capture session");
            m_framePool.Close();
            m_framePool = nullptr;
            return false;
        }

        m_frameArrivedToken = m_framePool.FrameArrived(
            [this](winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&)
        {
            winrt::Direct3D11CaptureFrame frame = nullptr;
            
            frame = sender.TryGetNextFrame();
            if (!frame) return;

            winrt::IDirect3DSurface surface = frame.Surface();
            if (!surface) return;

            winrt::com_ptr<ID3D11Texture2D> surfaceTexture = 
                GetDXGIInterfaceFromObject<ID3D11Texture2D>(surface);
            if (!surfaceTexture) return;

            std::lock_guard<std::mutex> lock(m_frameMutex);
            
            int idx = m_currentStagingIndex;
            if (m_stagingTextures[idx])
            {
                m_d3dContext->CopyResource(
                    m_stagingTextures[idx].get(), 
                    surfaceTexture.get());
                m_readableStagingIndex = idx;
                m_currentStagingIndex = 1 - idx;
            }
            
            m_frameCount++;
        });

        m_session.StartCapture();
        m_isCapturing = true;
        return true;
    }
    catch (const winrt::hresult_error& e)
    {
        std::stringstream ss;
        ss << "Start capture failed: " << winrt::to_string(e.message()) 
           << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    }
    catch (...)
    {
        setError("Unknown exception");
        return false;
    }
}

void WGCWindowCapture::StopContinuousCapture()
{
    if (!m_isCapturing) return;

    m_isCapturing = false;

    if (m_session)
    {
        try { m_session.Close(); } catch (...) {}
        m_session = nullptr;
    }

    if (m_framePool)
    {
        try { m_framePool.Close(); } catch (...) {}
        m_framePool = nullptr;
    }

    m_captureItem = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        for (int i = 0; i < 2; i++)
        {
            m_stagingTextures[i] = nullptr;
        }
        m_textureWidth = 0;
        m_textureHeight = 0;
        m_currentStagingIndex = 0;
        m_readableStagingIndex = -1;
    }
}

bool WGCWindowCapture::TryGetFrame(unsigned char** outData, int* outWidth, int* outHeight)
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    
    if (m_readableStagingIndex < 0 || !m_stagingTextures[m_readableStagingIndex])
    {
        return false;
    }

    auto& stagingTexture = m_stagingTextures[m_readableStagingIndex];
    
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_d3dContext->Map(stagingTexture.get(), 0, D3D11_MAP_READ, 0, &mapped);
    if (FAILED(hr)) return false;

    size_t dataSize = m_textureWidth * m_textureHeight * 4;

    *outData = static_cast<unsigned char*>(CoTaskMemAlloc(dataSize));
    if (!*outData)
    {
        m_d3dContext->Unmap(stagingTexture.get(), 0);
        return false;
    }

    size_t rowPitch = mapped.RowPitch;
    for (int y = 0; y < m_textureHeight; y++)
    {
        unsigned char* src = static_cast<unsigned char*>(mapped.pData) + y * rowPitch;
        unsigned char* dst = *outData + y * m_textureWidth * 4;
        memcpy(dst, src, m_textureWidth * 4);
    }

    m_d3dContext->Unmap(stagingTexture.get(), 0);

    *outWidth = m_textureWidth;
    *outHeight = m_textureHeight;
    
    return true;
}
