#include "pch.h"
#include "WindowEnumerator.h"

WindowEnumerator::WindowEnumerator()
{
}

WindowEnumerator::~WindowEnumerator()
{
}

std::vector<WindowInfo> WindowEnumerator::EnumerateVisibleWindows()
{
    std::vector<WindowInfo> windows;

    EnumWindows([](HWND hwnd, LPARAM lParam)
    {
        auto* windowList = reinterpret_cast<std::vector<WindowInfo>*>(lParam);
        
        if (GetWindowTextLengthW(hwnd) > 0)
        {
            auto titleLength = GetWindowTextLengthW(hwnd);
            if (titleLength > 0)
            {
                titleLength++;
            }
            std::wstring title(titleLength, 0);
            GetWindowTextW(hwnd, title.data(), titleLength);
            
            auto classNameLength = 256;
            std::wstring className(classNameLength, 0);
            GetClassNameW(hwnd, className.data(), classNameLength);
            
            WindowInfo info;
            info.WindowHandle = hwnd;
            info.Title = title;
            info.ClassName = className;
            
            if (WindowEnumerator::IsCapturableWindow(hwnd))
            {
                windowList->push_back(info);
            }
        }
        
        return TRUE;
    }, reinterpret_cast<LPARAM>(&windows));

    return windows;
}

HWND WindowEnumerator::FindWindowByTitleAndClass(const std::wstring& title, const std::wstring& className)
{
    FindWindowContext context;
    context.targetTitle = title;
    context.targetClassName = className;
    context.foundWindow = nullptr;

    EnumWindows(FindWindowProc, reinterpret_cast<LPARAM>(&context));

    return context.foundWindow;
}

BOOL CALLBACK WindowEnumerator::FindWindowProc(HWND hwnd, LPARAM lParam)
{
    auto* context = reinterpret_cast<FindWindowContext*>(lParam);

    if (GetWindowTextLengthW(hwnd) > 0)
    {
        auto titleLength = GetWindowTextLengthW(hwnd);
        if (titleLength > 0)
        {
            titleLength++;
        }
        std::wstring title(titleLength, 0);
        GetWindowTextW(hwnd, title.data(), titleLength);

        auto classNameLength = 256;
        std::wstring className(classNameLength, 0);
        GetClassNameW(hwnd, className.data(), classNameLength);

        // Trim whitespace from strings
        title.erase(title.find_last_not_of(L" \t\n\r") + 1);
        className.erase(className.find_last_not_of(L" \t\n\r") + 1);
        context->targetTitle.erase(context->targetTitle.find_last_not_of(L" \t\n\r") + 1);
        context->targetClassName.erase(context->targetClassName.find_last_not_of(L" \t\n\r") + 1);

        // Check if titles and class names match (case-insensitive)
        if (_wcsicmp(title.c_str(), context->targetTitle.c_str()) == 0 &&
            _wcsicmp(className.c_str(), context->targetClassName.c_str()) == 0)
        {
            context->foundWindow = hwnd;
            return FALSE;
        }
    }

    return TRUE;
}

bool WindowEnumerator::IsCapturableWindow(HWND hwnd)
{
    if (hwnd == GetShellWindow() ||
        !IsWindowVisible(hwnd) ||
        GetAncestor(hwnd, GA_ROOT) != hwnd)
    {
        return false;
    }

    auto style = GetWindowLongW(hwnd, GWL_STYLE);
    if (style & WS_DISABLED)
    {
        return false;
    }

    auto exStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW)
    {
        return false;
    }

    auto classNameLength = 256;
    std::wstring className(classNameLength, 0);
    GetClassNameW(hwnd, className.data(), classNameLength);

    if (wcscmp(className.c_str(), L"Windows.UI.Core.CoreWindow") == 0 ||
        wcscmp(className.c_str(), L"ApplicationFrameWindow") == 0)
    {
        DWORD cloaked = FALSE;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) &&
            (cloaked == DWM_CLOAKED_SHELL))
        {
            return false;
        }
    }

    if (WindowEnumerator::IsKnownBlockedWindow(hwnd))
    {
        return false;
    }

    return true;
}

bool WindowEnumerator::IsKnownBlockedWindow(HWND hwnd)
{
    auto titleLength = GetWindowTextLengthW(hwnd);
    if (titleLength > 0)
    {
        titleLength++;
    }
    std::wstring title(titleLength, 0);
    GetWindowTextW(hwnd, title.data(), titleLength);

    auto classNameLength = 256;
    std::wstring className(classNameLength, 0);
    GetClassNameW(hwnd, className.data(), classNameLength);

    return wcscmp(title.c_str(), L"Task View") == 0 &&
        wcscmp(className.c_str(), L"Windows.UI.Core.CoreWindow") == 0 ||
        wcscmp(title.c_str(), L"DesktopWindowXamlSource") == 0 &&
        wcscmp(className.c_str(), L"Windows.UI.Core.CoreWindow") == 0 ||
        wcscmp(title.c_str(), L"PopupHost") == 0 &&
        wcscmp(className.c_str(), L"Xaml_WindowedPopupClass") == 0;
}
