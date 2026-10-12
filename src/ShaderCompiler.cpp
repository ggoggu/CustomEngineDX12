#include "ShaderCompiler.h"
#include <stdexcept>
#include <vector>
#include <iostream>
#include <Windows.h>

inline void ThrowIfFailed(HRESULT hr) {
    if (FAILED(hr)) throw std::runtime_error("DXC failed");
}

ShaderCompiler::ShaderCompiler() {
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_utils)));
    ThrowIfFailed(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)));
    ThrowIfFailed(m_utils->CreateDefaultIncludeHandler(&m_includeHandler));
}

ComPtr<IDxcBlob> ShaderCompiler::CompileShader(
    const std::wstring& filename,
    const std::wstring& entryPoint,
    const std::wstring& targetProfile)
{
    ComPtr<IDxcBlobEncoding> sourceBlob;
    HRESULT hr = m_utils->LoadFile(filename.c_str(), nullptr, &sourceBlob);
    if (FAILED(hr)) {
        throw std::runtime_error("Could not load shader file");
    }

    DxcBuffer sourceBuffer;
    sourceBuffer.Ptr = sourceBlob->GetBufferPointer();
    sourceBuffer.Size = sourceBlob->GetBufferSize();
    sourceBuffer.Encoding = DXC_CP_ACP;

    std::vector<LPCWSTR> arguments = {
        filename.c_str(),
        L"-E", entryPoint.c_str(),
        L"-T", targetProfile.c_str(),
        L"-Zi", // Debug info
        L"-Qembed_debug", // Embed debug info into the shader
        L"-Od", // Disable optimization
    };

    ComPtr<IDxcResult> results;
    hr = m_compiler->Compile(
        &sourceBuffer,
        arguments.data(),
        (UINT32)arguments.size(),
        m_includeHandler.Get(),
        IID_PPV_ARGS(&results)
    );
    ThrowIfFailed(hr);

    ComPtr<IDxcBlobUtf8> errors;
    results->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors != nullptr && errors->GetStringLength() != 0) {
        OutputDebugStringA(errors->GetStringPointer());
        std::cerr << errors->GetStringPointer() << std::endl;
        throw std::runtime_error("Shader compilation failed");
    }

    HRESULT status;
    results->GetStatus(&status);
    ThrowIfFailed(status);

    ComPtr<IDxcBlob> shader;
    ThrowIfFailed(results->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shader), nullptr));

    return shader;
}
