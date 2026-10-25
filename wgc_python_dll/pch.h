#pragma once

#define NOMINMAX
#define WGC_CAPTURE_DLL_EXPORTS 

#include <Unknwn.h>
#include <inspectable.h>

#include <d3d11_4.h>
#include <dxgi1_6.h>

#include <wil/cppwinrt.h>

// wgc-python - Windows Graphics Capture Python Bindings
// Author: XuanChenxuan
// 
// Uses robmikh.common for D3D11 interop and desktop capture helpers
// https://github.com/robmikh/Win32CaptureSample
#include <robmikh.common/direct3d11.interop.h>
#include <robmikh.common/capture.desktop.interop.h>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>


#include <atomic>
#include <memory>
#include <algorithm>
#include <vector>
#include <mutex>
#include <string>

#include <dwmapi.h>



namespace util
{
    using robmikh::common::desktop::CreateCaptureItemForWindow;
    using robmikh::common::desktop::CreateCaptureItemForMonitor;

    inline winrt::com_ptr<ID3D11Device> CreateD3D11Device()
    {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        winrt::com_ptr<ID3D11Device> d3dDevice;
        winrt::com_ptr<ID3D11DeviceContext> d3dContext;

        winrt::check_hresult(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            &featureLevel,
            1,
            D3D11_SDK_VERSION,
            d3dDevice.put(),
            nullptr,
            d3dContext.put()));

        return d3dDevice;
    }
}
