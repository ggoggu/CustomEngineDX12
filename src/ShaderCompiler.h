#pragma once
#define NOMINMAX
#include <Windows.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>

using Microsoft::WRL::ComPtr;

class ShaderCompiler {
public:
    ShaderCompiler();
    ~ShaderCompiler() = default;

    ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filename,
        const std::wstring& entryPoint,
        const std::wstring& targetProfile
    );

private:
    ComPtr<IDxcUtils> m_utils;
    ComPtr<IDxcCompiler3> m_compiler;
    ComPtr<IDxcIncludeHandler> m_includeHandler;
};
