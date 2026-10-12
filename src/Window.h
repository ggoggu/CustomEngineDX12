#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <string>

class Window {
public:
    Window(int width, int height, const std::wstring& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool ProcessMessages();
    HWND GetHWND() const { return m_hwnd; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    bool IsRightMouseDown() const { return m_isRButtonDown; }
    bool ConsumeMouseDelta(int& outDx, int& outDy);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HWND m_hwnd;
    HINSTANCE m_hInstance;
    int m_width;
    int m_height;
    std::wstring m_title;
    std::wstring m_windowClassName;

    // Mouse input tracking
    bool m_isRButtonDown;
    int m_lastMouseX;
    int m_lastMouseY;
    int m_mouseDeltaX;
    int m_mouseDeltaY;
};
