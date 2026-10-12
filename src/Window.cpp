#include "Window.h"

Window::Window(int width, int height, const std::wstring& title)
    : m_width(width), m_height(height), m_title(title), m_windowClassName(L"CustomEngineWindowClass"),
      m_hwnd(nullptr), m_hInstance(nullptr),
      m_isRButtonDown(false), m_lastMouseX(0), m_lastMouseY(0), m_mouseDeltaX(0), m_mouseDeltaY(0)
{
    m_hInstance = GetModuleHandle(nullptr);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_OWNDC;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = m_hInstance;
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = m_windowClassName.c_str();
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

    RegisterClassExW(&wc);

    // Adjust window size so the client area matches the requested width/height
    RECT wr = { 0, 0, m_width, m_height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindowExW(
        0,
        m_windowClassName.c_str(),
        m_title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left,
        wr.bottom - wr.top,
        nullptr,
        nullptr,
        m_hInstance,
        this // Pass pointer to this class to use in WindowProc
    );

    ShowWindow(m_hwnd, SW_SHOW);
}

Window::~Window()
{
    if (m_isRButtonDown)
    {
        ReleaseCapture();
        m_isRButtonDown = false;
    }
    DestroyWindow(m_hwnd);
    UnregisterClassW(m_windowClassName.c_str(), m_hInstance);
}

bool Window::ConsumeMouseDelta(int& outDx, int& outDy)
{
    outDx = m_mouseDeltaX;
    outDy = m_mouseDeltaY;
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    return (outDx != 0 || outDy != 0);
}

bool Window::ProcessMessages()
{
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        if (msg.message == WM_QUIT)
        {
            return false;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return true;
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Window* pWindow = nullptr;

    if (uMsg == WM_NCCREATE)
    {
        CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
        pWindow = reinterpret_cast<Window*>(pCreate->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
    }
    else
    {
        pWindow = reinterpret_cast<Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (pWindow)
    {
        switch (uMsg)
        {
        case WM_RBUTTONDOWN:
        {
            SetCapture(hwnd);
            pWindow->m_isRButtonDown = true;
            pWindow->m_lastMouseX = static_cast<short>(LOWORD(lParam));
            pWindow->m_lastMouseY = static_cast<short>(HIWORD(lParam));
            return 0;
        }
        case WM_RBUTTONUP:
        {
            if (pWindow->m_isRButtonDown)
            {
                pWindow->m_isRButtonDown = false;
                ReleaseCapture();
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            if (pWindow->m_isRButtonDown)
            {
                short curX = static_cast<short>(LOWORD(lParam));
                short curY = static_cast<short>(HIWORD(lParam));
                pWindow->m_mouseDeltaX += (curX - pWindow->m_lastMouseX);
                pWindow->m_mouseDeltaY += (curY - pWindow->m_lastMouseY);
                pWindow->m_lastMouseX = curX;
                pWindow->m_lastMouseY = curY;
            }
            return 0;
        }
        case WM_CAPTURECHANGED:
        {
            pWindow->m_isRButtonDown = false;
            return 0;
        }
        case WM_KILLFOCUS:
        {
            if (pWindow->m_isRButtonDown)
            {
                pWindow->m_isRButtonDown = false;
                ReleaseCapture();
            }
            return 0;
        }
        case WM_CLOSE:
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
