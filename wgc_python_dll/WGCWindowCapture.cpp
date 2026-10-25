// wgc-python — Windows Graphics Capture for Python
// Capture core: double-buffered staging + zero-copy Map + Pause/Resume
//
// Copyright (c) 2026 XuanChenxuan
// MIT License — see LICENSE file for details

#include "pch.h"
#include "WGCWindowCapture.h"
#include <sstream>
#include <winrt/Windows.Foundation.Metadata.h>

namespace
{
    std::string HResultToString(HRESULT hr)
    {
        std::stringstream ss;
        ss << "0x" << std::hex << hr;
        return ss.str();
    }
}

WGCWindowCapture::WGCWindowCapture() : m_initialized(false)
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

    if (m_initialized) return true;

    try {
        auto d3dDevice = util::CreateD3D11Device();
        if (!d3dDevice) { setError("Failed to create D3D11 device"); return false; }

        auto dxgiDevice = d3dDevice.as<IDXGIDevice>();
        if (!dxgiDevice) { setError("Failed to get DXGI device"); return false; }

        m_device = CreateDirect3DDevice(dxgiDevice.get());
        if (!m_device) { setError("Failed to create Direct3D device"); return false; }

        m_d3dDevice = GetDXGIInterfaceFromObject<ID3D11Device>(m_device);
        if (!m_d3dDevice) { setError("Failed to get D3D11 device interface"); return false; }

        m_d3dDevice->GetImmediateContext(m_d3dContext.put());
        if (!m_d3dContext) { setError("Failed to get device context"); return false; }

        m_initialized = true;
        return true;
    } catch (const winrt::hresult_error& e) {
        std::stringstream ss;
        ss << "WinRT error: " << winrt::to_string(e.message()) << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    } catch (...) {
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

    for (int i = 0; i < 2; i++) {
        if (FAILED(m_d3dDevice->CreateTexture2D(&desc, nullptr, m_stagingTextures[i].put()))) return false;
    }

    m_textureWidth = width;
    m_textureHeight = height;
    m_currentStagingIndex = 0;
    m_readableStagingIndex = -1;
    return true;
}

void WGCWindowCapture::CalcClientRectOffset(HWND hwnd, int& offsetX, int& offsetY, int& clientW, int& clientH)
{
    RECT windowRect, clientRect;
    GetWindowRect(hwnd, &windowRect);
    GetClientRect(hwnd, &clientRect);
    
    POINT pt = {0, 0};
    ClientToScreen(hwnd, &pt);
    
    offsetX = pt.x - windowRect.left;
    offsetY = pt.y - windowRect.top;
    clientW = clientRect.right - clientRect.left;
    clientH = clientRect.bottom - clientRect.top;

    int virtualW = windowRect.right - windowRect.left;
    if (m_sourceWidth > 0 && m_sourceWidth != virtualW) {
        float scale = static_cast<float>(m_sourceWidth) / static_cast<float>(virtualW);
        offsetX = static_cast<int>(offsetX * scale + 0.5f);
        offsetY = static_cast<int>(offsetY * scale + 0.5f);
        clientW = static_cast<int>(clientW * scale + 0.5f);
        clientH = static_cast<int>(clientH * scale + 0.5f);
    }
}

bool WGCWindowCapture::StartContinuousCapture(HWND hwnd, bool clientAreaOnly, std::string* outError)
{
    auto setError = [&](const std::string& msg) { if (outError) *outError = msg; };

    if (!m_initialized) { setError("Not initialized"); return false; }
    if (m_isCapturing) StopContinuousCapture();

    try {
        auto item = util::CreateCaptureItemForWindow(hwnd);
        if (!item) { setError("Failed to create capture item"); return false; }
        return StartContinuousCaptureCommon(item, hwnd, clientAreaOnly, outError);
    } catch (const winrt::hresult_error& e) {
        std::stringstream ss;
        ss << "Start capture failed: " << winrt::to_string(e.message()) << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    } catch (...) {
        setError("Unknown exception");
        return false;
    }
}

bool WGCWindowCapture::StartContinuousCaptureMonitor(HMONITOR hmon, std::string* outError)
{
    auto setError = [&](const std::string& msg) { if (outError) *outError = msg; };

    if (!m_initialized) { setError("Not initialized"); return false; }
    if (m_isCapturing) StopContinuousCapture();

    try {
        auto item = util::CreateCaptureItemForMonitor(hmon);
        if (!item) { setError("Failed to create capture item"); return false; }
        // 显示器捕获始终取全屏内容（无客户区裁剪语义，m_clientAreaOnly=false）
        return StartContinuousCaptureCommon(item, nullptr, false, outError);
    } catch (const winrt::hresult_error& e) {
        std::stringstream ss;
        ss << "Start monitor capture failed: " << winrt::to_string(e.message()) << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    } catch (...) {
        setError("Unknown exception");
        return false;
    }
}

bool WGCWindowCapture::StartContinuousCaptureCommon(
    winrt::GraphicsCaptureItem const& item,
    HWND hwnd,
    bool clientAreaOnly,
    std::string* outError)
{
    auto setError = [&](const std::string& msg) { if (outError) *outError = msg; };

    try {
        if (!item) { setError("Invalid capture item"); return false; }

        auto size = item.Size();
        if (size.Width <= 0 || size.Height <= 0) { setError("Invalid capture item size"); return false; }

        m_sourceWidth = static_cast<int>(size.Width);
        m_sourceHeight = static_cast<int>(size.Height);
        m_clientAreaOnly = clientAreaOnly;
        m_hwnd = hwnd;

        int texWidth = m_sourceWidth;
        int texHeight = m_sourceHeight;

        if (clientAreaOnly) {
            if (!hwnd) { setError("Client-area capture requires a window handle"); return false; }
            int clientW, clientH;
            CalcClientRectOffset(hwnd, m_cropX, m_cropY, clientW, clientH);
            texWidth = clientW;
            texHeight = clientH;
        }

        if (texWidth <= 0 || texHeight <= 0) {
            setError("Window is minimized or has zero client area (client area "
                     + std::to_string(texWidth) + "x" + std::to_string(texHeight) + ")");
            return false;
        }

        if (!CreateTextures(texWidth, texHeight)) { setError("Failed to create textures"); return false; }

        m_framePool = winrt::Direct3D11CaptureFramePool::CreateFreeThreaded(
            m_device, winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, size);
        if (!m_framePool) { setError("Failed to create frame pool"); return false; }

        m_session = m_framePool.CreateCaptureSession(m_captureItem);
        ApplyCursorCaptureSetting();

        m_frameArrivedAlive = std::make_shared<std::atomic<bool>>(true);
        auto alive = m_frameArrivedAlive;
        m_framePool.FrameArrived([this, alive](winrt::Direct3D11CaptureFramePool const& sender, winrt::IInspectable const&) {
            auto frame = sender.TryGetNextFrame();
            auto frameSize = frame.ContentSize();
            auto surfaceTexture = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());

            std::lock_guard<std::mutex> lock(m_frameMutex);
            if (!alive->load()) return;

            UINT newW = static_cast<UINT>(frameSize.Width);
            UINT newH = static_cast<UINT>(frameSize.Height);
            if (newW != m_sourceWidth || newH != m_sourceHeight) {
                m_sourceWidth = newW;
                m_sourceHeight = newH;
                int texW = newW, texH = newH;
                if (m_clientAreaOnly) {
                    int clientW, clientH;
                    CalcClientRectOffset(m_hwnd, m_cropX, m_cropY, clientW, clientH);
                    texW = clientW;
                    texH = clientH;
                }
                for (int i = 0; i < 2; i++) {
                    if (m_textureInUse[i]) m_d3dContext->Unmap(m_stagingTextures[i].get(), 0);
                    m_stagingTextures[i] = nullptr;
                    m_textureInUse[i] = false;
                }
                m_readableStagingIndex = -1;
                m_currentStagingIndex = 0;
                if (!CreateTextures(texW, texH)) return;
                sender.Recreate(m_device, winrt::DirectXPixelFormat::B8G8R8A8UIntNormalized, 2, frameSize);
                return;
            }

            if (m_isPaused.load()) return;

            int idx = m_currentStagingIndex;
            if (m_textureInUse[idx]) return;

            if (m_clientAreaOnly) {
                D3D11_BOX srcBox = {};
                srcBox.left = m_cropX;
                srcBox.top = m_cropY;
                srcBox.right = m_cropX + m_textureWidth;
                srcBox.bottom = m_cropY + m_textureHeight;
                srcBox.front = 0;
                srcBox.back = 1;
                m_d3dContext->CopySubresourceRegion(
                    m_stagingTextures[idx].get(), 0, 0, 0, 0,
                    surfaceTexture.get(), 0, &srcBox);
            } else {
                m_d3dContext->CopyResource(m_stagingTextures[idx].get(), surfaceTexture.get());
            }
            m_readableStagingIndex = idx;
            m_currentStagingIndex = 1 - idx;
            m_frameCount++;
        });

        m_session.StartCapture();
        m_isCapturing.store(true);
        m_isPaused.store(false);
        return true;
    } catch (const winrt::hresult_error& e) {
        std::stringstream ss;
        ss << "Start capture failed: " << winrt::to_string(e.message()) << " (HRESULT: " << HResultToString(e.code()) << ")";
        setError(ss.str());
        return false;
    } catch (...) {
        setError("Unknown exception");
        return false;
    }
}

void WGCWindowCapture::PauseCapture()
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_isPaused.store(true);
    m_readableStagingIndex = -1;
}

void WGCWindowCapture::ResumeCapture()
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    m_isPaused.store(false);
}

void WGCWindowCapture::SetCursorCaptureEnabled(bool enabled)
{
    m_captureCursor = enabled;
    ApplyCursorCaptureSetting();
}

// IsCursorCaptureEnabled 需要 Windows 10 2004 (19041)+，低版本系统静默跳过
void WGCWindowCapture::ApplyCursorCaptureSetting()
{
    if (!m_session) return;
    try {
        if (winrt::Windows::Foundation::Metadata::ApiInformation::IsPropertyPresent(
                L"Windows.Graphics.Capture.GraphicsCaptureSession", L"IsCursorCaptureEnabled"))
        {
            m_session.IsCursorCaptureEnabled(m_captureCursor);
        }
    } catch (...) {}
}

void WGCWindowCapture::StopContinuousCapture()
{
    if (m_frameArrivedAlive) *m_frameArrivedAlive = false;

    if (m_session) { try { m_session.Close(); } catch (...) {} m_session = nullptr; }
    if (m_framePool) { try { m_framePool.Close(); } catch (...) {} m_framePool = nullptr; }
    m_captureItem = nullptr;

    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        for (int i = 0; i < 2; i++) {
            if (m_textureInUse[i]) m_d3dContext->Unmap(m_stagingTextures[i].get(), 0);
            m_stagingTextures[i] = nullptr;
            m_textureInUse[i] = false;
        }
    }

    m_frameArrivedAlive = nullptr;
    m_textureWidth = 0;
    m_textureHeight = 0;
    m_cropX = m_cropY = 0;
    m_sourceWidth = m_sourceHeight = 0;
    m_currentStagingIndex = 0;
    m_readableStagingIndex = -1;
    m_mappedIndex = -1;
    m_hwnd = nullptr;
    m_isCapturing.store(false);
    m_isPaused.store(false);
}

bool WGCWindowCapture::MapFrame(unsigned char** outData, int* outWidth, int* outHeight, int* outRowPitch)
{
    int idx;
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        if (m_readableStagingIndex < 0) return false;
        if (m_isPaused.load()) return false;
        idx = m_readableStagingIndex;
        m_textureInUse[idx] = true;
        m_mappedIndex = idx;
    }

    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(m_d3dContext->Map(m_stagingTextures[idx].get(), 0, D3D11_MAP_READ, 0, &mapped)))
    {
        std::lock_guard<std::mutex> lock(m_frameMutex);
        m_textureInUse[idx] = false;
        m_mappedIndex = -1;
        return false;
    }

    *outData = static_cast<unsigned char*>(mapped.pData);
    *outWidth = m_textureWidth;
    *outHeight = m_textureHeight;
    if (outRowPitch) *outRowPitch = static_cast<int>(mapped.RowPitch);
    return true;
}

void WGCWindowCapture::ReleaseFrame()
{
    std::lock_guard<std::mutex> lock(m_frameMutex);
    int idx = m_mappedIndex;
    m_mappedIndex = -1;
    if (idx < 0) return;
    if (!m_textureInUse[idx]) return;
    m_d3dContext->Unmap(m_stagingTextures[idx].get(), 0);
    m_textureInUse[idx] = false;
}
