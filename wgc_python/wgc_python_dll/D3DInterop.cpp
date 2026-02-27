#include "pch.h"

extern "C"
{
    HRESULT __stdcall CreateDirect3D11DeviceFromDXGIDevice(::IDXGIDevice* dxgiDevice,
        ::IInspectable** graphicsDevice)
    {
        using CreateDirect3D11DeviceFromDXGIDeviceProc = HRESULT(__stdcall*)(
            ::IDXGIDevice* dxgiDevice,
            ::IInspectable** graphicsDevice);

        static CreateDirect3D11DeviceFromDXGIDeviceProc proc = nullptr;
        static std::once_flag onceFlag;
        static HMODULE d3d11 = nullptr;

        std::call_once(onceFlag, []() {
            d3d11 = LoadLibraryEx(L"d3d11.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (d3d11)
            {
                proc = reinterpret_cast<CreateDirect3D11DeviceFromDXGIDeviceProc>(
                    GetProcAddress(d3d11, "CreateDirect3D11DeviceFromDXGIDevice"));
            }
        });

        if (!d3d11 || !proc)
        {
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }

        return proc(dxgiDevice, graphicsDevice);
    }

    HRESULT __stdcall CreateDirect3D11SurfaceFromDXGISurface(::IDXGISurface* dxgiSurface,
        ::IInspectable** graphicsSurface)
    {
        using CreateDirect3D11SurfaceFromDXGISurfaceProc = HRESULT(__stdcall*)(
            ::IDXGISurface* dxgiSurface,
            ::IInspectable** graphicsSurface);

        static CreateDirect3D11SurfaceFromDXGISurfaceProc proc = nullptr;
        static std::once_flag onceFlag;
        static HMODULE d3d11 = nullptr;

        std::call_once(onceFlag, []() {
            d3d11 = LoadLibraryEx(L"d3d11.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (d3d11)
            {
                proc = reinterpret_cast<CreateDirect3D11SurfaceFromDXGISurfaceProc>(
                    GetProcAddress(d3d11, "CreateDirect3D11SurfaceFromDXGISurface"));
            }
        });

        if (!d3d11 || !proc)
        {
            return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
        }

        return proc(dxgiSurface, graphicsSurface);
    }
}
