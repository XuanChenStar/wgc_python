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
#include <winrt/Windows.Foundation.Metadata.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>
#include <winrt/Windows.Graphics.Imaging.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>

#include <atomic>
#include <memory>
#include <algorithm>
#include <unordered_set>
#include <vector>
#include <optional>
#include <future>
#include <mutex>
#include <string>

#include <wincodec.h>
#include <dwmapi.h>
#include <shcore.h>
#include <wincrypt.h>

#include <wil/resource.h>
#include <wil/cppwinrt_helpers.h>
#include <wil/coroutine.h>

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

    inline winrt::com_ptr<ID3D11Texture2D> CopyD3DTexture(
        winrt::com_ptr<ID3D11Device> const& device,
        winrt::com_ptr<ID3D11Texture2D> const& texture,
        bool includeAlpha)
    {
        D3D11_TEXTURE2D_DESC desc = {};
        texture->GetDesc(&desc);

        D3D11_TEXTURE2D_DESC copyDesc = desc;
        copyDesc.Usage = D3D11_USAGE_DEFAULT;
        copyDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        copyDesc.CPUAccessFlags = 0;

        winrt::com_ptr<ID3D11Texture2D> copy;
        winrt::check_hresult(device->CreateTexture2D(&copyDesc, nullptr, copy.put()));

        winrt::com_ptr<ID3D11DeviceContext> context;
        device->GetImmediateContext(context.put());
        context->CopyResource(copy.get(), texture.get());

        return copy;
    }
}
