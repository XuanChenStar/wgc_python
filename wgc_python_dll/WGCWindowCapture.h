// wgc-python — Windows Graphics Capture for Python
// Capture core declarations
//
// Copyright (c) 2026 XuanChenxuan
// MIT License — see LICENSE file for details

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

    bool StartContinuousCapture(HWND hwnd, bool clientAreaOnly, std::string* outError = nullptr);
    // 捕获目标窗口所在显示器的全部内容（捕获会话绑定显示器，不随目标窗口全屏/窗口切换失效）
    bool StartContinuousCaptureMonitor(HMONITOR hmon, std::string* outError = nullptr);
    void StopContinuousCapture();
    bool MapFrame(unsigned char** outData, int* outWidth, int* outHeight, int* outRowPitch = nullptr);
    void ReleaseFrame();
    void SetCursorCaptureEnabled(bool enabled);
    
    bool IsCapturing() const { return m_isCapturing; }
    int GetFrameCount() const { return m_frameCount.load(); }
    
    void PauseCapture();
    void ResumeCapture();
    bool IsPaused() const { return m_isPaused; }

    winrt::com_ptr<ID3D11Device> m_d3dDevice;

private:
    winrt::com_ptr<ID3D11DeviceContext> m_d3dContext;
    winrt::IDirect3DDevice m_device;
    bool m_initialized;

    HWND m_hwnd = nullptr;
    winrt::GraphicsCaptureItem m_captureItem{ nullptr };
    winrt::Direct3D11CaptureFramePool m_framePool{ nullptr };
    winrt::GraphicsCaptureSession m_session{ nullptr };
    
    std::mutex m_frameMutex;
    
    winrt::com_ptr<ID3D11Texture2D> m_stagingTextures[2];
    int m_currentStagingIndex = 0;
    int m_readableStagingIndex = -1;
    bool m_textureInUse[2] = {false, false};
    int m_mappedIndex = -1;
    
    int m_textureWidth = 0;
    int m_textureHeight = 0;
    
    int m_cropX = 0;
    int m_cropY = 0;
    int m_sourceWidth = 0;
    int m_sourceHeight = 0;
    bool m_clientAreaOnly = false;
    bool m_captureCursor = true;
    
    std::atomic<int> m_frameCount{0};
    std::atomic<bool> m_isCapturing{false};
    std::atomic<bool> m_isPaused{false};
    std::shared_ptr<std::atomic<bool>> m_frameArrivedAlive;
    
    bool CreateTextures(UINT width, UINT height);
    void CalcClientRectOffset(HWND hwnd, int& offsetX, int& offsetY, int& clientW, int& clientH);
    bool StartContinuousCaptureCommon(
        winrt::GraphicsCaptureItem const& item,
        HWND hwnd,
        bool clientAreaOnly,
        std::string* outError = nullptr);
    void ApplyCursorCaptureSetting();
};
