#include "Window.h"
#include "DX12Renderer.h"
#include <memory>
#include <Windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
    (void)hInstance; (void)hPrevInstance; (void)pCmdLine; (void)nCmdShow;
    LPWSTR fullCmdLine = GetCommandLineW();
    bool isSmokeTest = (fullCmdLine && (wcsstr(fullCmdLine, L"--smoke-test") != nullptr || wcsstr(fullCmdLine, L"-smoke") != nullptr));
    int smokeFrameCount = 0;
    const int maxSmokeFrames = 5;

    try
    {
        Window window(1280, 720, L"Custom Engine - DX12 Path Tracer");
        std::unique_ptr<DX12Renderer> renderer = std::make_unique<DX12Renderer>(window.GetHWND(), window.GetWidth(), window.GetHeight());

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        LARGE_INTEGER lastTime;
        QueryPerformanceCounter(&lastTime);

        bool isRunning = true;
        while (isRunning)
        {
            if (!window.ProcessMessages())
            {
                isRunning = false;
                break;
            }

            LARGE_INTEGER currentTime;
            QueryPerformanceCounter(&currentTime);
            float deltaTime = static_cast<float>(currentTime.QuadPart - lastTime.QuadPart) / frequency.QuadPart;
            lastTime = currentTime;

            renderer->Update(deltaTime);
            renderer->Render();

            if (isSmokeTest)
            {
                smokeFrameCount++;
                if (smokeFrameCount >= maxSmokeFrames)
                {
                    break;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        OutputDebugStringA(e.what());
        FILE* fp = nullptr;
        fopen_s(&fp, "smoke_error.txt", "w");
        if (fp) { fputs(e.what(), fp); fclose(fp); }
        return 1;
    }
    catch (...)
    {
        OutputDebugStringA("Unknown fatal exception occurred.\n");
        FILE* fp = nullptr;
        fopen_s(&fp, "smoke_error.txt", "w");
        if (fp) { fputs("Unknown fatal exception occurred.\n", fp); fclose(fp); }
        return 1;
    }

    return 0;
}
