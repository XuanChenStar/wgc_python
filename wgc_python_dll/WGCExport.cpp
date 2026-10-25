// wgc-python — Windows Graphics Capture for Python
// DLL exports — thread-safe multi-instance capture
//
// Copyright (c) 2026 XuanChenxuan
// MIT License — see LICENSE file for details

#include "pch.h"
#include "WGCExport.h"
#include "WindowEnumerator.h"
#include "WGCWindowCapture.h"
#include <unordered_map>

static std::string g_lastError;
static std::mutex g_errorMutex;
static std::mutex g_managerMutex;
static std::unordered_map<CaptureHandle, std::unique_ptr<WGCWindowCapture>> g_captures;
static std::atomic<uint64_t> g_nextHandle{1};
static std::atomic<bool> g_winrtInit{false};
static std::mutex g_initMutex;

static std::wstring UTF8ToWStr(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring r(n - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, r.data(), n);
    return r;
}

static std::string WStrToUTF8(const std::wstring& s) {
    if (s.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string r(n - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, r.data(), n, nullptr, nullptr);
    return r;
}

static void SetError(const std::string& msg) {
    std::lock_guard<std::mutex> lk(g_errorMutex);
    g_lastError = msg;
}

static std::string GetError() {
    std::lock_guard<std::mutex> lk(g_errorMutex);
    return g_lastError;
}

static bool InitWinRT() {
    if (g_winrtInit) return true;
    std::lock_guard<std::mutex> lk(g_initMutex);
    if (g_winrtInit) return true;
    try { winrt::init_apartment(winrt::apartment_type::multi_threaded); g_winrtInit = true; return true; }
    catch (...) { return false; }
}

static HWND FindWnd(const char* title, const char* cls) {
    auto t = UTF8ToWStr(title), c = UTF8ToWStr(cls);
    WindowEnumerator e;
    HWND h = e.FindWindowByTitleAndClass(t, c);
    return h;
}

WGC_API const char* GetLastErrorMsg() {
    thread_local std::string buf;
    buf = GetError();
    return buf.c_str();
}

WGC_API int EnumerateWindows(char*** titles, char*** classNames, int* count) {
    try {
        WindowEnumerator e;
        auto wins = e.EnumerateVisibleWindows();
        *count = (int)wins.size();
        if (wins.empty()) { *titles = *classNames = nullptr; return 1; }
        *titles = (char**)CoTaskMemAlloc(sizeof(char*) * wins.size());
        *classNames = (char**)CoTaskMemAlloc(sizeof(char*) * wins.size());
        for (size_t i = 0; i < wins.size(); i++) {
            auto t = WStrToUTF8(wins[i].Title), c = WStrToUTF8(wins[i].ClassName);
            (*titles)[i] = (char*)CoTaskMemAlloc(t.size() + 1);
            (*classNames)[i] = (char*)CoTaskMemAlloc(c.size() + 1);
            strcpy_s((*titles)[i], t.size() + 1, t.c_str());
            strcpy_s((*classNames)[i], c.size() + 1, c.c_str());
        }
        return 1;
    } catch (...) { return 0; }
}

WGC_API void FreeStringArray(char** arr, int cnt) {
    for (int i = 0; i < cnt; i++) CoTaskMemFree(arr[i]);
    CoTaskMemFree(arr);
}

WGC_API CaptureHandle StartCapture(const char* title, const char* className, int clientAreaOnly) {
    try {
        SetError("");
        if (!InitWinRT()) { SetError("WinRT init failed"); return 0; }
        HWND h = FindWnd(title, className);
        if (!h) { SetError("Window not found"); return 0; }
        if (!IsWindowVisible(h)) { SetError("Window not visible"); return 0; }
        
        auto cap = std::make_unique<WGCWindowCapture>();
        std::string err;
        if (!cap->Initialize(&err)) { SetError("Init failed: " + err); return 0; }
        if (!cap->StartContinuousCapture(h, clientAreaOnly != 0, &err)) { SetError("Start failed: " + err); return 0; }
        
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto handle = g_nextHandle.fetch_add(1);
        g_captures[handle] = std::move(cap);
        return handle;
    } catch (...) { SetError("Exception"); return 0; }
}

WGC_API CaptureHandle StartCaptureMonitorOfWindow(const char* title, const char* className) {
    try {
        SetError("");
        if (!InitWinRT()) { SetError("WinRT init failed"); return 0; }
        HWND h = FindWnd(title, className);
        if (!h) { SetError("Window not found"); return 0; }
        // 捕获目标窗口所在显示器（会话桌面）：捕获会话绑定显示器，
        // 不随目标窗口全屏/窗口切换失效
        HMONITOR hmon = MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST);
        if (!hmon) { SetError("Monitor not found"); return 0; }

        auto cap = std::make_unique<WGCWindowCapture>();
        std::string err;
        if (!cap->Initialize(&err)) { SetError("Init failed: " + err); return 0; }
        if (!cap->StartContinuousCaptureMonitor(hmon, &err)) { SetError("Start failed: " + err); return 0; }
        // 自动化坐标不应受鼠标光标干扰（与窗口捕获的 capture_cursor=false 同语义）；
        // 需 Win10 2004+，低版本静默跳过（光标会保留在帧中）
        cap->SetCursorCaptureEnabled(false);
        
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto handle = g_nextHandle.fetch_add(1);
        g_captures[handle] = std::move(cap);
        return handle;
    } catch (...) { SetError("Exception"); return 0; }
}

WGC_API void SetCursorCaptureEnabled(CaptureHandle h, int enabled) {
    if (!h) return;
    std::lock_guard<std::mutex> lk(g_managerMutex);
    auto it = g_captures.find(h);
    if (it != g_captures.end()) it->second->SetCursorCaptureEnabled(enabled != 0);
}

WGC_API void DestroyCapture(CaptureHandle h) {
    try {
        if (!h) return;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        if (it != g_captures.end()) {
            it->second->StopContinuousCapture();
            it->second->Cleanup();
            g_captures.erase(it);
        }
    } catch (...) {}
}

WGC_API int GetFrameMapped(CaptureHandle h, unsigned char** data, int* w, int* h2, int* rowPitch) {
    try {
        if (!h) return 0;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        if (it == g_captures.end()) return 0;
        return it->second->MapFrame(data, w, h2, rowPitch) ? 1 : 0;
    } catch (...) { return 0; }
}

WGC_API void ReleaseMappedFrame(CaptureHandle h) {
    try {
        if (!h) return;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        if (it != g_captures.end()) it->second->ReleaseFrame();
    } catch (...) {}
}

WGC_API void StopCapture(CaptureHandle h) {
    try {
        if (!h) return;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        if (it != g_captures.end()) it->second->StopContinuousCapture();
    } catch (...) {}
}

WGC_API void PauseCapture(CaptureHandle h) {
    if (!h) return;
    std::lock_guard<std::mutex> lk(g_managerMutex);
    auto it = g_captures.find(h);
    if (it != g_captures.end()) it->second->PauseCapture();
}

WGC_API void ResumeCapture(CaptureHandle h) {
    if (!h) return;
    std::lock_guard<std::mutex> lk(g_managerMutex);
    auto it = g_captures.find(h);
    if (it != g_captures.end()) it->second->ResumeCapture();
}

WGC_API int IsCapturing(CaptureHandle h) {
    try {
        if (!h) return 0;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        return (it != g_captures.end()) ? (it->second->IsCapturing() ? 1 : 0) : 0;
    } catch (...) { return 0; }
}

WGC_API int IsPaused(CaptureHandle h) {
    try {
        if (!h) return 0;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        return (it != g_captures.end()) ? (it->second->IsPaused() ? 1 : 0) : 0;
    } catch (...) { return 0; }
}

WGC_API int GetFrameCount(CaptureHandle h) {
    try {
        if (!h) return 0;
        std::lock_guard<std::mutex> lk(g_managerMutex);
        auto it = g_captures.find(h);
        return (it != g_captures.end()) ? it->second->GetFrameCount() : 0;
    } catch (...) { return 0; }
}

WGC_API size_t GetActiveCaptureCount() {
    std::lock_guard<std::mutex> lk(g_managerMutex);
    return g_captures.size();
}
