#include "pch.h"
#include "WGCExport.h"
#include "WindowEnumerator.h"
#include "WGCWindowCapture.h"
#include <memory>
#include <atomic>

static std::unique_ptr<WGCWindowCapture> g_capture = nullptr;
static std::mutex g_captureMutex;
static std::string g_lastErrorMsg = "";
static std::mutex g_errorMsgMutex;
static bool g_winrtInitialized = false;
static std::mutex g_initMutex;

static bool EnsureWinRTInitialized()
{
    std::lock_guard<std::mutex> lock(g_initMutex);
    
    if (g_winrtInitialized) return true;
    
    try
    {
        winrt::init_apartment(winrt::apartment_type::single_threaded);
        g_winrtInitialized = true;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

static std::string WStringToUTF8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, result.data(), size, nullptr, nullptr);
    return result;
}

static std::wstring UTF8ToWString(const std::string& str)
{
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static void SetLastErrorMsg(const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_errorMsgMutex);
    g_lastErrorMsg = msg;
}

static HWND FindTargetWindow(const char* title, const char* className)
{
    std::wstring titleW = UTF8ToWString(title);
    std::wstring classNameW = UTF8ToWString(className);

    WindowEnumerator enumerator;
    HWND hwnd = enumerator.FindWindowByTitleAndClass(titleW, classNameW);

    if (!hwnd) hwnd = FindWindow(nullptr, titleW.c_str());

    return hwnd;
}

static bool EnsureCaptureInitialized()
{
    if (!g_capture)
    {
        g_capture = std::make_unique<WGCWindowCapture>();
        std::string err;
        if (!g_capture->Initialize(&err))
        {
            SetLastErrorMsg("Init failed: " + err);
            g_capture = nullptr;
            return false;
        }
    }
    return true;
}

// === DLL Exports ===

WGC_API const char* GetLastErrorMsg()
{
    std::lock_guard<std::mutex> lock(g_errorMsgMutex);
    return g_lastErrorMsg.c_str();
}

WGC_API int EnumerateWindows(char*** titles, char*** classNames, int* count)
{
    try
    {
        WindowEnumerator enumerator;
        auto windows = enumerator.EnumerateVisibleWindows();

        *count = static_cast<int>(windows.size());

        if (windows.empty())
        {
            *titles = nullptr;
            *classNames = nullptr;
            return 1;
        }

        *titles = static_cast<char**>(CoTaskMemAlloc(sizeof(char*) * windows.size()));
        *classNames = static_cast<char**>(CoTaskMemAlloc(sizeof(char*) * windows.size()));

        for (size_t i = 0; i < windows.size(); i++)
        {
            std::string title = WStringToUTF8(windows[i].Title);
            std::string className = WStringToUTF8(windows[i].ClassName);

            (*titles)[i] = static_cast<char*>(CoTaskMemAlloc(title.size() + 1));
            strcpy_s((*titles)[i], title.size() + 1, title.c_str());

            (*classNames)[i] = static_cast<char*>(CoTaskMemAlloc(className.size() + 1));
            strcpy_s((*classNames)[i], className.size() + 1, className.c_str());
        }

        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

WGC_API void FreeStringArray(char** array, int count)
{
    if (!array) return;
    for (int i = 0; i < count; i++)
    {
        if (array[i]) CoTaskMemFree(array[i]);
    }
    CoTaskMemFree(array);
}

WGC_API int StartContinuousCapture(const char* title, const char* className)
{
    try
    {
        std::lock_guard<std::mutex> lock(g_captureMutex);
        SetLastErrorMsg("");

        if (!EnsureWinRTInitialized())
        {
            SetLastErrorMsg("WinRT init failed");
            return 0;
        }

        if (!EnsureCaptureInitialized()) return 0;

        HWND hwnd = FindTargetWindow(title, className);
        if (!hwnd)
        {
            SetLastErrorMsg("Window not found");
            return 0;
        }

        if (!IsWindowVisible(hwnd))
        {
            SetLastErrorMsg("Window not visible");
            return 0;
        }

        std::string err;
        if (!g_capture->StartContinuousCapture(hwnd, &err))
        {
            SetLastErrorMsg("Start capture failed: " + err);
            return 0;
        }

        return 1;
    }
    catch (...)
    {
        SetLastErrorMsg("Unknown exception");
        return 0;
    }
}

WGC_API int GetLatestFrame(unsigned char** imageData, int* width, int* height)
{
    try
    {
        std::lock_guard<std::mutex> lock(g_captureMutex);

        if (!g_capture || !g_capture->IsCapturing()) return 0;

        unsigned char* data = nullptr;
        int w = 0, h = 0;

        if (!g_capture->TryGetFrame(&data, &w, &h)) return 0;

        *imageData = data;
        *width = w;
        *height = h;

        return 1;
    }
    catch (...)
    {
        return 0;
    }
}

WGC_API void FreeImageData(unsigned char* data)
{
    if (data) CoTaskMemFree(data);
}

WGC_API void StopContinuousCapture()
{
    std::lock_guard<std::mutex> lock(g_captureMutex);
    if (g_capture) g_capture->StopContinuousCapture();
}

WGC_API int IsCapturing()
{
    std::lock_guard<std::mutex> lock(g_captureMutex);
    return (g_capture && g_capture->IsCapturing()) ? 1 : 0;
}

WGC_API int GetFrameCount()
{
    std::lock_guard<std::mutex> lock(g_captureMutex);
    return g_capture ? g_capture->GetFrameCount() : 0;
}
