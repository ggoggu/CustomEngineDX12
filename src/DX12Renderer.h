#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <stdexcept>
#include <cstdint>
#include <vector>
#include "ShaderCompiler.h"
#include "Camera.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

#include <sstream>
#include <iomanip>

#define ThrowIfFailed(hr) ThrowIfFailedHelper(hr, __FILE__, __LINE__)

inline void ThrowIfFailedHelper(HRESULT hr, const char* file, int line)
{
    if (FAILED(hr))
    {
        std::stringstream ss;
        ss << "DirectX 12 API failed with HRESULT 0x" << std::hex << hr << " at " << file << ":" << std::dec << line;
        throw std::runtime_error(ss.str());
    }
}

struct Vertex
{
    XMFLOAT3 position;
    XMFLOAT4 color;
};

struct ConstantBuffer
{
    XMMATRIX viewInverse;
    XMMATRIX projInverse;
    UINT frameCount;
    float padding[3];
};

class DX12Renderer {
public:
    DX12Renderer(HWND hwnd, int width, int height);
    ~DX12Renderer();

    void Update(float deltaTime);
    void Render();

private:
    void InitDX12();
    void LoadAssetsRaster();
    void LoadAssetsDXR();
    void LoadModel();
    void CreateAccelerationStructures();
    void CreateRaytracingPipeline();
    void CreateShaderBindingTable();
    void FlushCommandQueue();

    ComPtr<ID3D12Resource> CreateDefaultBuffer(const void* data, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer);

    HWND m_hwnd;
    int m_width;
    int m_height;
    Camera m_camera;

    // DXGI & D3D12 Core
    ComPtr<IDXGIFactory4> m_dxgiFactory;
    ComPtr<ID3D12Device5> m_device; // DXR requires Device5
    ComPtr<IDXGISwapChain3> m_swapChain;

    // Command Queue, Allocator, List
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12GraphicsCommandList4> m_commandList; // DXR requires CommandList4

    // Descriptor Heaps
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    UINT m_rtvDescriptorSize;

    // Render Targets
    static const uint8_t FrameCount = 2;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    UINT m_frameIndex;
    UINT m_accumulationFrames; // Track how many frames we've accumulated

    // Pipeline & Assets
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    
    ComPtr<ID3D12Resource> m_vertexBuffer;
    ComPtr<ID3D12Resource> m_vertexBufferUpload;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

    ComPtr<ID3D12Resource> m_indexBuffer;
    ComPtr<ID3D12Resource> m_indexBufferUpload;
    D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
    UINT m_indexCount;

    ComPtr<ID3D12Resource> m_constantBuffer;
    ConstantBuffer* m_cbvDataBegin;
    ShaderCompiler m_shaderCompiler;

    // DXR Acceleration Structures
    ComPtr<ID3D12Resource> m_bottomLevelAS;
    ComPtr<ID3D12Resource> m_topLevelAS;
    ComPtr<ID3D12Resource> m_instanceDescs;
    ComPtr<ID3D12Resource> m_scratchBuffer;

    // DXR Pipeline & SBT
    ComPtr<ID3D12StateObject> m_dxrStateObject;
    ComPtr<ID3D12Resource> m_shaderBindingTable;
    UINT m_sbtEntrySize;

    // DXR Output Texture
    ComPtr<ID3D12Resource> m_raytracingOutput;
    ComPtr<ID3D12Resource> m_accumulationBuffer;
    ComPtr<ID3D12DescriptorHeap> m_srvUavHeap;
    UINT m_srvUavDescriptorSize;

    // Viewport & Scissor Rect
    D3D12_VIEWPORT m_viewport;
    D3D12_RECT m_scissorRect;

    // Synchronization (Fence)
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValue;
    HANDLE m_fenceEvent;
};
