#pragma once
#include "pch.h"

struct WindowInfo
{
    HWND WindowHandle;
    std::wstring Title;
    std::wstring ClassName;
};

class WindowEnumerator
{
public:
    WindowEnumerator();
    ~WindowEnumerator();

    std::vector<WindowInfo> EnumerateVisibleWindows();
    HWND FindWindowByTitleAndClass(const std::wstring& title, const std::wstring& className);

private:
    static bool IsCapturableWindow(HWND hwnd);
    static bool IsKnownBlockedWindow(HWND hwnd);
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
    static BOOL CALLBACK FindWindowProc(HWND hwnd, LPARAM lParam);

    struct FindWindowContext
    {
        std::wstring targetTitle;
        std::wstring targetClassName;
        HWND foundWindow;
    };
};
