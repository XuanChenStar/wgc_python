#pragma once
#include "pch.h"

namespace winrt
{
    using namespace Windows::Foundation;
    using namespace Windows::Graphics;
    using namespace Windows::Graphics::Capture;
    using namespace Windows::Graphics::DirectX;
    using namespace Windows::Graphics::DirectX::Direct3D11;
}

class WGCWindowCapture
{
public:
    WGCWindowCapture();
    ~WGCWindowCapture();

    bool Initialize(std::string* outError = nullptr);
    void Cleanup();

    bool StartContinuousCapture(HWND hwnd, std::string* outError = nullptr);
    void StopContinuousCapture();
    bool TryGetFrame(unsigned char** outData, int* outWidth, int* outHeight);
    
    bool IsCapturing() const { return m_isCapturing; }
    int GetFrameCount() const { return m_frameCount.load(); }

    winrt::com_ptr<ID3D11Device> m_d3dDevice;

private:
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext;
    winrt::IDirect3DDevice m_device;
    bool m_initialized;

    winrt::GraphicsCaptureItem m_captureItem{ nullptr };
    winrt::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::GraphicsCaptureSession m_session{ nullptr };
    
    std::mutex m_frameMutex;
    
    winrt::com_ptr<ID3D11Texture2D> m_stagingTextures[2];
    int m_currentStagingIndex = 0;
    int m_readableStagingIndex = -1;
    
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    
    std::atomic<int> m_frameCount{0};
    bool m_isCapturing = false;
    winrt::event_token m_frameArrivedToken;
    
    bool CreateTextures(UINT width, UINT height);
};
