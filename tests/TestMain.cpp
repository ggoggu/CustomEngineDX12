#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <DirectXMath.h>
#include "../src/Camera.h"
#include "../src/ShaderCompiler.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

// Simple test framework macro
static int g_testsRun = 0;
static int g_testsPassed = 0;

#define RUN_TEST(testFunc) \
    do { \
        g_testsRun++; \
        std::cout << "[ RUN      ] " << #testFunc << std::endl; \
        try { \
            testFunc(); \
            g_testsPassed++; \
            std::cout << "[       OK ] " << #testFunc << std::endl; \
        } catch (const std::exception& e) { \
            std::cerr << "[  FAILED  ] " << #testFunc << " -> Exception: " << e.what() << std::endl; \
        } catch (...) { \
            std::cerr << "[  FAILED  ] " << #testFunc << " -> Unknown exception" << std::endl; \
        } \
    } while (0)

#define ASSERT_TRUE(condition, message) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + message); \
    }

#define ASSERT_NEAR(a, b, epsilon, message) \
    if (std::fabs((a) - (b)) > (epsilon)) { \
        throw std::runtime_error(std::string("Near assertion failed: ") + message + " (diff: " + std::to_string(std::fabs((a)-(b))) + ")"); \
    }

// -------------------------------------------------------------
// Test 1: Camera Math & Vector Orthogonality
// -------------------------------------------------------------
void Test_CameraMath()
{
    Camera camera;
    camera.SetPosition(0.0f, 2.0f, -10.0f);
    camera.SetRotation(0.0f, 0.0f);

    XMMATRIX view = camera.GetViewMatrix();
    XMMATRIX proj = camera.GetProjectionMatrix(16.0f / 9.0f);

    XMVECTOR detView;
    XMMATRIX viewInv = XMMatrixInverse(&detView, view);
    ASSERT_TRUE(XMVectorGetX(detView) != 0.0f, "View matrix determinant must not be zero");

    XMVECTOR detProj;
    XMMATRIX projInv = XMMatrixInverse(&detProj, proj);
    ASSERT_TRUE(XMVectorGetX(detProj) != 0.0f, "Projection matrix determinant must not be zero");

    // Check View * ViewInverse ~ Identity
    XMMATRIX identityCandidate = XMMatrixMultiply(view, viewInv);
    XMFLOAT4X4 idMat;
    XMStoreFloat4x4(&idMat, identityCandidate);

    for (int row = 0; row < 4; ++row)
    {
        for (int col = 0; col < 4; ++col)
        {
            float expected = (row == col) ? 1.0f : 0.0f;
            ASSERT_NEAR(idMat.m[row][col], expected, 0.001f, "Matrix element [" + std::to_string(row) + "][" + std::to_string(col) + "] should match identity");
        }
    }
}

// -------------------------------------------------------------
// Test 2: Camera Jump & Vertical Gravity Physics
// -------------------------------------------------------------
void Test_CameraJump()
{
    Camera camera;
    camera.SetPosition(0.0f, 0.0f, -5.0f);
    ASSERT_TRUE(camera.IsGrounded(), "Camera should be grounded initially");
    ASSERT_NEAR(camera.GetVerticalVelocity(), 0.0f, 0.001f, "Vertical velocity should be 0 initially");

    // Trigger jump
    camera.Jump();
    ASSERT_TRUE(!camera.IsGrounded(), "Camera should not be grounded immediately after jump");
    ASSERT_NEAR(camera.GetVerticalVelocity(), 7.0f, 0.001f, "Vertical velocity should match jump speed");

    // Advance 0.1s
    bool moved = camera.UpdatePhysics(0.1f);
    ASSERT_TRUE(moved, "UpdatePhysics should report moved while airborne");
    ASSERT_TRUE(camera.GetPosition().y > 0.0f, "Camera Y should be above ground during jump");
    ASSERT_TRUE(camera.GetVerticalVelocity() < 7.0f, "Gravity should decelerate vertical velocity");

    // Advance full arc until landing (gravity 20, v0 7 -> ~0.7s total)
    for (int i = 0; i < 20; ++i)
    {
        camera.UpdatePhysics(0.05f);
    }

    ASSERT_TRUE(camera.IsGrounded(), "Camera should be grounded after full jump trajectory");
    ASSERT_NEAR(camera.GetPosition().y, 0.0f, 0.001f, "Camera Y should return to ground level");
    ASSERT_NEAR(camera.GetVerticalVelocity(), 0.0f, 0.001f, "Vertical velocity should reset to 0 upon landing");
}

// -------------------------------------------------------------
// Test 2: Runtime Shader Compilation via DXC
// -------------------------------------------------------------
void Test_ShaderCompilation()
{
    ShaderCompiler compiler;

    // 1. Compile Raytracing Library
    ComPtr<IDxcBlob> rtBlob = compiler.CompileShader(L"shaders/raytracing.hlsl", L"", L"lib_6_5");
    ASSERT_TRUE(rtBlob != nullptr, "Raytracing shader blob must be valid");
    ASSERT_TRUE(rtBlob->GetBufferSize() > 0, "Raytracing shader byte code size must be > 0");

    // 2. Compile Raster Vertex Shader
    ComPtr<IDxcBlob> vsBlob = compiler.CompileShader(L"shaders/shaders.hlsl", L"VSMain", L"vs_6_5");
    ASSERT_TRUE(vsBlob != nullptr, "Vertex shader blob must be valid");
    ASSERT_TRUE(vsBlob->GetBufferSize() > 0, "Vertex shader byte code size must be > 0");

    // 3. Compile Raster Pixel Shader
    ComPtr<IDxcBlob> psBlob = compiler.CompileShader(L"shaders/shaders.hlsl", L"PSMain", L"ps_6_5");
    ASSERT_TRUE(psBlob != nullptr, "Pixel shader blob must be valid");
    ASSERT_TRUE(psBlob->GetBufferSize() > 0, "Pixel shader byte code size must be > 0");
}

// -------------------------------------------------------------
// Test 3: DirectX 12 Device & DXR Tier 1.1 Support
// -------------------------------------------------------------
void Test_DX12DeviceAndDXRFeatures()
{
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
    ASSERT_TRUE(SUCCEEDED(hr), "Failed to create DXGI Factory");

    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<ID3D12Device5> device;

    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&device))))
        {
            break;
        }
    }

    ASSERT_TRUE(device != nullptr, "D3D12 Device5 creation failed on available hardware adapters");

    // Check DXR Support
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
    hr = device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5));
    ASSERT_TRUE(SUCCEEDED(hr), "CheckFeatureSupport for D3D12_OPTIONS5 failed");
    ASSERT_TRUE(options5.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED, "GPU must support DirectX Raytracing (DXR)");

    // Test Command Allocator & List Creation
    ComPtr<ID3D12CommandAllocator> allocator;
    hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator));
    ASSERT_TRUE(SUCCEEDED(hr), "Failed to create ID3D12CommandAllocator");

    ComPtr<ID3D12GraphicsCommandList4> cmdList;
    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&cmdList));
    ASSERT_TRUE(SUCCEEDED(hr), "Failed to create ID3D12GraphicsCommandList4");

    hr = cmdList->Close();
    ASSERT_TRUE(SUCCEEDED(hr), "Failed to close ID3D12GraphicsCommandList4");
}

// -------------------------------------------------------------
// Test 4: Shader Binding Table Alignment Invariants
// -------------------------------------------------------------
void Test_SBTAlignmentInvariants()
{
    // Alignment constants defined by D3D12 Raytracing spec
    static_assert(D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT == 32, "Record alignment must be 32 bytes");
    static_assert(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT == 64, "Table alignment must be 64 bytes");

    auto Align = [](UINT size, UINT alignment) {
        return (size + (alignment - 1)) & ~(alignment - 1);
    };

    // Test with sample shader identifier + 8 bytes local root args
    UINT rawRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + 8; // 32 + 8 = 40 bytes
    UINT alignedRecordSize = Align(rawRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
    ASSERT_TRUE(alignedRecordSize % 32 == 0, "Aligned record size must be a multiple of 32");
    ASSERT_TRUE(alignedRecordSize >= rawRecordSize, "Aligned record size must be >= raw size");
    ASSERT_TRUE(alignedRecordSize == 64, "40 bytes aligned to 32 should equal 64 bytes");

    UINT numRecords = 3;
    UINT rawTableSize = alignedRecordSize * numRecords; // 192 bytes
    UINT alignedTableSize = Align(rawTableSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
    ASSERT_TRUE(alignedTableSize % 64 == 0, "Aligned table size must be a multiple of 64");
    ASSERT_TRUE(alignedTableSize == 192, "192 bytes is already a multiple of 64");
}

// -------------------------------------------------------------
// Main Test Runner Entry Point
// -------------------------------------------------------------
int main()
{
    std::cout << "======================================================" << std::endl;
    std::cout << " Running CustomEngine Automated Test Suite (C++20/DX12)" << std::endl;
    std::cout << "======================================================" << std::endl;

    RUN_TEST(Test_CameraMath);
    RUN_TEST(Test_CameraJump);
    RUN_TEST(Test_ShaderCompilation);
    RUN_TEST(Test_DX12DeviceAndDXRFeatures);
    RUN_TEST(Test_SBTAlignmentInvariants);

    std::cout << "======================================================" << std::endl;
    std::cout << " Test Summary: " << g_testsPassed << "/" << g_testsRun << " Passed" << std::endl;
    std::cout << "======================================================" << std::endl;

    return (g_testsPassed == g_testsRun) ? 0 : 1;
}
