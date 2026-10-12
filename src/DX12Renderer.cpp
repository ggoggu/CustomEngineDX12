#define NOMINMAX
#include "DX12Renderer.h"
#include <iostream>
#include <iterator>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

DX12Renderer::DX12Renderer(HWND hwnd, int width, int height)
    : m_hwnd(hwnd), m_width(width), m_height(height), m_frameIndex(0), m_accumulationFrames(0), m_fenceValue(0), m_cbvDataBegin(nullptr), m_indexCount(0)
{
    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = static_cast<float>(width);
    m_viewport.Height = static_cast<float>(height);
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    m_scissorRect.left = 0;
    m_scissorRect.top = 0;
    m_scissorRect.right = width;
    m_scissorRect.bottom = height;

    InitDX12();
    LoadAssetsDXR();
    LoadModel();
    CreateAccelerationStructures();
    CreateRaytracingPipeline();
    CreateShaderBindingTable();

    // Close and execute the initialization command list
    ThrowIfFailed(m_commandList->Close());
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    FlushCommandQueue();
}

DX12Renderer::~DX12Renderer()
{
    FlushCommandQueue();
    if (m_constantBuffer)
        m_constantBuffer->Unmap(0, nullptr);
    CloseHandle(m_fenceEvent);
}

ComPtr<ID3D12Resource> DX12Renderer::CreateDefaultBuffer(const void* data, UINT64 byteSize, ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    D3D12_HEAP_PROPERTIES heapPropsDefault = {};
    heapPropsDefault.Type = D3D12_HEAP_TYPE_DEFAULT;
    
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = byteSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapPropsDefault,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&defaultBuffer)));

    D3D12_HEAP_PROPERTIES heapPropsUpload = {};
    heapPropsUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapPropsUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&uploadBuffer)));

    UINT8* pData;
    D3D12_RANGE readRange = { 0, 0 };
    ThrowIfFailed(uploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pData)));
    memcpy(pData, data, byteSize);
    uploadBuffer->Unmap(0, nullptr);

    m_commandList->CopyBufferRegion(defaultBuffer.Get(), 0, uploadBuffer.Get(), 0, byteSize);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = defaultBuffer.Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    m_commandList->ResourceBarrier(1, &barrier);

    return defaultBuffer;
}

void DX12Renderer::InitDX12()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
    {
        debugController->EnableDebugLayer();
    }
#endif

    UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_dxgiFactory)));

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT i = 0; m_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        ComPtr<ID3D12Device5> testDevice;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&testDevice))))
        {
            D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupportData = {};
            if (SUCCEEDED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupportData, sizeof(featureSupportData))) &&
                featureSupportData.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED)
            {
                m_device = testDevice;
                break;
            }
        }
    }

    if (!m_device)
    {
        throw std::runtime_error("No DXR-capable DirectX 12 hardware adapter found.");
    }

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }
#endif

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = FrameCount;
    swapChainDesc.Width = m_width;
    swapChainDesc.Height = m_height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(m_dxgiFactory->CreateSwapChainForHwnd(
        m_commandQueue.Get(),
        m_hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    ));

    ThrowIfFailed(swapChain.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = FrameCount;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT n = 0; n < FrameCount; n++)
    {
        ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n])));
        m_device->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
        rtvHandle.ptr += m_rtvDescriptorSize;
    }

    ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));

    ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)));
    
    // Do NOT close command list here.

    ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceValue = 1;

    m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (m_fenceEvent == nullptr)
    {
        ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
    }
}

void DX12Renderer::LoadAssetsRaster()
{
    // 1. Create root signature with a Root Descriptor for CBV
    D3D12_ROOT_PARAMETER rootParameters[1];
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].Descriptor.RegisterSpace = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.NumParameters = 1;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    // 2. Compile shaders using DXC (Shader Model 6.0)
    ComPtr<IDxcBlob> vertexShader = m_shaderCompiler.CompileShader(L"shaders/shaders.hlsl", L"VSMain", L"vs_6_0");
    ComPtr<IDxcBlob> pixelShader = m_shaderCompiler.CompileShader(L"shaders/shaders.hlsl", L"PSMain", L"ps_6_0");

    // 3. Define the vertex input layout
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // 4. Create the Pipeline State Object (PSO)
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature = m_rootSignature.Get();
    psoDesc.VS = { reinterpret_cast<UINT8*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    psoDesc.PS = { reinterpret_cast<UINT8*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Disable culling for spinning triangle
    psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
    psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    psoDesc.RasterizerState.DepthClipEnable = TRUE;
    psoDesc.RasterizerState.MultisampleEnable = FALSE;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

    // 6. Create Constant Buffer
    D3D12_HEAP_PROPERTIES heapPropsUpload = {};
    heapPropsUpload.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapPropsUpload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapPropsUpload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapPropsUpload.CreationNodeMask = 1;
    heapPropsUpload.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = (sizeof(ConstantBuffer) + 255) & ~255; // CB size is required to be 256-byte aligned
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapPropsUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)));

    D3D12_RANGE readRange = { 0, 0 };
    ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)));
}

void DX12Renderer::LoadAssetsDXR()
{
    // 1. Create Global Root Signature
    D3D12_DESCRIPTOR_RANGE descriptorRanges[2] = {};
    
    // UAVs (u0 for RenderTarget, u1 for AccumulationBuffer)
    descriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    descriptorRanges[0].NumDescriptors = 2; // Two UAVs!
    descriptorRanges[0].BaseShaderRegister = 0;
    descriptorRanges[0].RegisterSpace = 0;
    descriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    // SRV (t0) for Acceleration Structure
    descriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRanges[1].NumDescriptors = 1;
    descriptorRanges[1].BaseShaderRegister = 0;
    descriptorRanges[1].RegisterSpace = 0;
    descriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[5] = {};
    // RenderTarget + AccumulationBuffer (u0, u1)
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].DescriptorTable.pDescriptorRanges = &descriptorRanges[0];
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Scene (t0)
    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].Descriptor.RegisterSpace = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Vertices (t1)
    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[2].Descriptor.ShaderRegister = 1;
    rootParameters[2].Descriptor.RegisterSpace = 0;
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // Indices (t2)
    rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
    rootParameters[3].Descriptor.ShaderRegister = 2;
    rootParameters[3].Descriptor.RegisterSpace = 0;
    rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    // CameraCB (b0)
    rootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[4].Descriptor.ShaderRegister = 0;
    rootParameters[4].Descriptor.RegisterSpace = 0;
    rootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.NumParameters = 5;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumStaticSamplers = 0;
    rootSignatureDesc.pStaticSamplers = nullptr;
    rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));

    // 2. Create UAV Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC srvUavHeapDesc = {};
    srvUavHeapDesc.NumDescriptors = 3; // 2 for UAVs, 1 for extra
    srvUavHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvUavHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    ThrowIfFailed(m_device->CreateDescriptorHeap(&srvUavHeapDesc, IID_PPV_ARGS(&m_srvUavHeap)));
    m_srvUavDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 3. Create Raytracing Output & Accumulation Textures (UAVs)
    D3D12_RESOURCE_DESC uavDesc = {};
    uavDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    uavDesc.Alignment = 0;
    uavDesc.Width = m_width;
    uavDesc.Height = m_height;
    uavDesc.DepthOrArraySize = 1;
    uavDesc.MipLevels = 1;
    uavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // High precision for accumulation!
    uavDesc.SampleDesc.Count = 1;
    uavDesc.SampleDesc.Quality = 0;
    uavDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    uavDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    // Accumulation buffer (float4)
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &uavDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&m_accumulationBuffer)));

    // Output buffer MUST match swapchain format (DXGI_FORMAT_R8G8B8A8_UNORM) for CopyResource to succeed
    D3D12_RESOURCE_DESC outputDesc = uavDesc;
    outputDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &outputDesc,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        nullptr,
        IID_PPV_ARGS(&m_raytracingOutput)));

    D3D12_UNORDERED_ACCESS_VIEW_DESC outputUavDesc = {};
    outputUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    outputUavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    
    D3D12_CPU_DESCRIPTOR_HANDLE srvUavHandle = m_srvUavHeap->GetCPUDescriptorHandleForHeapStart();
    m_device->CreateUnorderedAccessView(m_raytracingOutput.Get(), nullptr, &outputUavDesc, srvUavHandle);
    
    D3D12_UNORDERED_ACCESS_VIEW_DESC accumUavDesc = {};
    accumUavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
    accumUavDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;

    srvUavHandle.ptr += m_srvUavDescriptorSize;
    m_device->CreateUnorderedAccessView(m_accumulationBuffer.Get(), nullptr, &accumUavDesc, srvUavHandle);

    // 4. Create Constant Buffer
    D3D12_HEAP_PROPERTIES heapPropsUpload = {};
    heapPropsUpload.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = (sizeof(ConstantBuffer) + 255) & ~255;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &heapPropsUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_constantBuffer)));

    D3D12_RANGE readRange = { 0, 0 };
    ThrowIfFailed(m_constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&m_cbvDataBegin)));
}

void DX12Renderer::CreateRaytracingPipeline()
{
    ComPtr<IDxcBlob> raytracingShader = m_shaderCompiler.CompileShader(L"shaders/raytracing.hlsl", L"", L"lib_6_3");

    D3D12_STATE_OBJECT_DESC raytracingPipelineDesc = {};
    raytracingPipelineDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

    D3D12_STATE_SUBOBJECT subobjects[5] = {};

    // 1. DXIL Library
    D3D12_DXIL_LIBRARY_DESC libDesc = {};
    libDesc.DXILLibrary.pShaderBytecode = raytracingShader->GetBufferPointer();
    libDesc.DXILLibrary.BytecodeLength = raytracingShader->GetBufferSize();
    libDesc.NumExports = 0; // Export all
    libDesc.pExports = nullptr;

    subobjects[0].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
    subobjects[0].pDesc = &libDesc;

    // 2. Hit Group
    D3D12_HIT_GROUP_DESC hitGroupDesc = {};
    hitGroupDesc.HitGroupExport = L"HitGroup";
    hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
    hitGroupDesc.ClosestHitShaderImport = L"ClosestHit";
    hitGroupDesc.AnyHitShaderImport = nullptr;
    hitGroupDesc.IntersectionShaderImport = nullptr;

    subobjects[1].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
    subobjects[1].pDesc = &hitGroupDesc;

    // 3. Shader Config
    D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
    shaderConfig.MaxPayloadSizeInBytes = 10 * sizeof(float); // 40 bytes: float3 color (12) + float hitT (4) + float3 normal (12) + float3 albedo (12)
    shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float); // 8 bytes: float2 barycentrics

    subobjects[2].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
    subobjects[2].pDesc = &shaderConfig;

    // 4. Global Root Signature
    D3D12_GLOBAL_ROOT_SIGNATURE globalRootSig = {};
    globalRootSig.pGlobalRootSignature = m_rootSignature.Get();

    subobjects[3].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
    subobjects[3].pDesc = &globalRootSig;

    // 5. Pipeline Config
    D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig = {};
    pipelineConfig.MaxTraceRecursionDepth = 1; // 1 ray bounce (primary rays only)

    subobjects[4].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
    subobjects[4].pDesc = &pipelineConfig;

    raytracingPipelineDesc.NumSubobjects = 5;
    raytracingPipelineDesc.pSubobjects = subobjects;

    ThrowIfFailed(m_device->CreateStateObject(&raytracingPipelineDesc, IID_PPV_ARGS(&m_dxrStateObject)));
}

void DX12Renderer::CreateShaderBindingTable()
{
    ComPtr<ID3D12StateObjectProperties> stateObjectProperties;
    ThrowIfFailed(m_dxrStateObject.As(&stateObjectProperties));

    void* rayGenShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"RayGen");
    void* missShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"Miss");
    void* hitGroupShaderIdentifier = stateObjectProperties->GetShaderIdentifier(L"HitGroup");

    m_sbtEntrySize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    m_sbtEntrySize = (m_sbtEntrySize + (D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT - 1)) & ~(D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT - 1); // Align to 64 bytes

    UINT sbtSize = m_sbtEntrySize * 3;

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Alignment = 0;
    bufferDesc.Width = sbtSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.SampleDesc.Quality = 0;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_shaderBindingTable)));

    uint8_t* pData;
    ThrowIfFailed(m_shaderBindingTable->Map(0, nullptr, reinterpret_cast<void**>(&pData)));
    
    memcpy(pData, rayGenShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(pData + m_sbtEntrySize, missShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    memcpy(pData + m_sbtEntrySize * 2, hitGroupShaderIdentifier, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
    
    m_shaderBindingTable->Unmap(0, nullptr);
}

void DX12Renderer::LoadModel()
{
    std::vector<XMFLOAT3> temp_vertices;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    std::ifstream file("assets/cube.obj");
    if (!file.is_open()) {
        throw std::runtime_error("Could not open assets/cube.obj");
    }

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string type;
        iss >> type;
        
        if (type == "v") {
            XMFLOAT3 v;
            iss >> v.x >> v.y >> v.z;
            temp_vertices.push_back(v);
        } else if (type == "f") {
            for (int i = 0; i < 4; ++i) { // Our cube has quads
                std::string vertexData;
                iss >> vertexData;
                if (vertexData.empty()) break;
                
                std::istringstream viss(vertexData);
                std::string vIdxStr;
                std::getline(viss, vIdxStr, '/'); // Get vertex index
                
                uint32_t vIdx = std::stoi(vIdxStr) - 1;
                
                Vertex vertex = {};
                vertex.position = temp_vertices[vIdx];
                vertex.color = { vertex.position.x + 0.5f, vertex.position.y + 0.5f, vertex.position.z + 0.5f, 1.0f };
                
                vertices.push_back(vertex);
            }
            
            // Triangulate the quad (0 1 2 3 -> 0 1 2, 0 2 3)
            uint32_t baseIdx = static_cast<uint32_t>(vertices.size()) - 4;
            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 1);
            indices.push_back(baseIdx + 2);
            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 2);
            indices.push_back(baseIdx + 3);
        }
    }

    m_indexCount = static_cast<UINT>(indices.size());
    const UINT vertexBufferSize = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    const UINT indexBufferSize = static_cast<UINT>(indices.size() * sizeof(uint32_t));

    m_vertexBuffer = CreateDefaultBuffer(vertices.data(), vertexBufferSize, m_vertexBufferUpload);
    m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
    m_vertexBufferView.StrideInBytes = sizeof(Vertex);
    m_vertexBufferView.SizeInBytes = vertexBufferSize;

    m_indexBuffer = CreateDefaultBuffer(indices.data(), indexBufferSize, m_indexBufferUpload);
    m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
    m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
    m_indexBufferView.SizeInBytes = indexBufferSize;
}

void DX12Renderer::CreateAccelerationStructures()
{
    // 1. Build BLAS (Bottom Level Acceleration Structure)
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc = {};
    geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer = m_indexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount = m_indexCount;
    geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
    geometryDesc.Triangles.Transform3x4 = 0;
    geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geometryDesc.Triangles.VertexCount = m_indexCount; // Just to be safe, vertexCount should be number of vertices
    // Actually, m_vertexBufferView.SizeInBytes / sizeof(Vertex) is the correct vertex count.
    geometryDesc.Triangles.VertexCount = m_vertexBufferView.SizeInBytes / sizeof(Vertex);
    geometryDesc.Triangles.VertexBuffer.StartAddress = m_vertexBuffer->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
    geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInputs = {};
    blasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    blasInputs.pGeometryDescs = &geometryDesc;
    blasInputs.NumDescs = 1;
    blasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO blasPrebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&blasInputs, &blasPrebuildInfo);

    // 2. Build TLAS (Top Level Acceleration Structure)
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS tlasInputs = {};
    tlasInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlasInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    tlasInputs.NumDescs = 1;
    tlasInputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;

    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO tlasPrebuildInfo = {};
    m_device->GetRaytracingAccelerationStructurePrebuildInfo(&tlasInputs, &tlasPrebuildInfo);

    // 3. Allocate Scratch Buffer
    UINT64 scratchSize = std::max(blasPrebuildInfo.ScratchDataSizeInBytes, tlasPrebuildInfo.ScratchDataSizeInBytes);

    D3D12_HEAP_PROPERTIES defaultHeap = {};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC scratchDesc = {};
    scratchDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    scratchDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
    scratchDesc.Width = scratchSize;
    scratchDesc.Height = 1;
    scratchDesc.DepthOrArraySize = 1;
    scratchDesc.MipLevels = 1;
    scratchDesc.Format = DXGI_FORMAT_UNKNOWN;
    scratchDesc.SampleDesc.Count = 1;
    scratchDesc.SampleDesc.Quality = 0;
    scratchDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    scratchDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &scratchDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&m_scratchBuffer)));

    // 4. Allocate Result Buffers (BLAS and TLAS)
    D3D12_RESOURCE_DESC blasDesc = scratchDesc;
    blasDesc.Width = blasPrebuildInfo.ResultDataMaxSizeInBytes;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &blasDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&m_bottomLevelAS)));

    D3D12_RESOURCE_DESC tlasDesc = scratchDesc;
    tlasDesc.Width = tlasPrebuildInfo.ResultDataMaxSizeInBytes;
    ThrowIfFailed(m_device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &tlasDesc,
        D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
        nullptr,
        IID_PPV_ARGS(&m_topLevelAS)));

    // 5. Create Instance Descriptor for TLAS
    D3D12_RAYTRACING_INSTANCE_DESC instanceDesc = {};
    instanceDesc.Transform[0][0] = 1.0f; instanceDesc.Transform[0][1] = 0.0f; instanceDesc.Transform[0][2] = 0.0f; instanceDesc.Transform[0][3] = 0.0f;
    instanceDesc.Transform[1][0] = 0.0f; instanceDesc.Transform[1][1] = 1.0f; instanceDesc.Transform[1][2] = 0.0f; instanceDesc.Transform[1][3] = 0.0f;
    instanceDesc.Transform[2][0] = 0.0f; instanceDesc.Transform[2][1] = 0.0f; instanceDesc.Transform[2][2] = 1.0f; instanceDesc.Transform[2][3] = 0.0f;
    instanceDesc.InstanceID = 0;
    instanceDesc.InstanceMask = 0xFF;
    instanceDesc.InstanceContributionToHitGroupIndex = 0;
    instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
    instanceDesc.AccelerationStructure = m_bottomLevelAS->GetGPUVirtualAddress();

    D3D12_HEAP_PROPERTIES uploadHeap = {};
    uploadHeap.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC instanceDescDesc = scratchDesc;
    instanceDescDesc.Width = sizeof(instanceDesc);
    instanceDescDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    ThrowIfFailed(m_device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &instanceDescDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_instanceDescs)));

    void* pMappedData;
    ThrowIfFailed(m_instanceDescs->Map(0, nullptr, &pMappedData));
    memcpy(pMappedData, &instanceDesc, sizeof(instanceDesc));
    m_instanceDescs->Unmap(0, nullptr);

    tlasInputs.InstanceDescs = m_instanceDescs->GetGPUVirtualAddress();

    // 6. Build BLAS
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC buildDesc = {};
    buildDesc.Inputs = blasInputs;
    buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = m_bottomLevelAS->GetGPUVirtualAddress();

    m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);

    // Barrier to wait for BLAS build to finish
    D3D12_RESOURCE_BARRIER uavBarrier = {};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource = m_bottomLevelAS.Get();
    m_commandList->ResourceBarrier(1, &uavBarrier);

    // 7. Build TLAS
    buildDesc.Inputs = tlasInputs;
    buildDesc.ScratchAccelerationStructureData = m_scratchBuffer->GetGPUVirtualAddress();
    buildDesc.DestAccelerationStructureData = m_topLevelAS->GetGPUVirtualAddress();

    m_commandList->BuildRaytracingAccelerationStructure(&buildDesc, 0, nullptr);
    
    // Barrier to wait for TLAS build to finish
    uavBarrier.UAV.pResource = m_topLevelAS.Get();
    m_commandList->ResourceBarrier(1, &uavBarrier);
}

void DX12Renderer::Update(float deltaTime, int mouseDeltaX, int mouseDeltaY)
{
    if (m_camera.Update(deltaTime, mouseDeltaX, mouseDeltaY)) {
        m_accumulationFrames = 0;
    }
    m_accumulationFrames++;

    XMMATRIX view = m_camera.GetViewMatrix();
    XMMATRIX proj = m_camera.GetProjectionMatrix(static_cast<float>(m_width) / m_height);

    XMVECTOR det;
    m_cbvDataBegin->viewInverse = XMMatrixTranspose(XMMatrixInverse(&det, view));
    m_cbvDataBegin->projInverse = XMMatrixTranspose(XMMatrixInverse(&det, proj));
    m_cbvDataBegin->frameCount = m_accumulationFrames;
}

void DX12Renderer::Render()
{
    ThrowIfFailed(m_commandAllocator->Reset());
    ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

    ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvUavHeap.Get() };
    m_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

    m_commandList->SetComputeRootSignature(m_rootSignature.Get());

    // Bind UAV (Render Target)
    m_commandList->SetComputeRootDescriptorTable(0, m_srvUavHeap->GetGPUDescriptorHandleForHeapStart());
    
    // Bind TLAS (SRV)
    m_commandList->SetComputeRootShaderResourceView(1, m_topLevelAS->GetGPUVirtualAddress());

    // Bind CBV (b0) -> Parameter 4
    m_commandList->SetComputeRootConstantBufferView(4, m_constantBuffer->GetGPUVirtualAddress());

    // Bind Vertices (t1) -> Parameter 2
    m_commandList->SetComputeRootShaderResourceView(2, m_vertexBuffer->GetGPUVirtualAddress());

    // Bind Indices (t2) -> Parameter 3
    m_commandList->SetComputeRootShaderResourceView(3, m_indexBuffer->GetGPUVirtualAddress());

    // Transition raytracing output to UNORDERED_ACCESS and synchronize accumulation buffer
    D3D12_RESOURCE_BARRIER preRaytraceBarriers[2] = {};
    preRaytraceBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    preRaytraceBarriers[0].Transition.pResource = m_raytracingOutput.Get();
    preRaytraceBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    preRaytraceBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;

    preRaytraceBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    preRaytraceBarriers[1].UAV.pResource = m_accumulationBuffer.Get();
    m_commandList->ResourceBarrier(2, preRaytraceBarriers);

    m_commandList->SetPipelineState1(m_dxrStateObject.Get());

    D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
    dispatchDesc.HitGroupTable.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress() + m_sbtEntrySize * 2;
    dispatchDesc.HitGroupTable.SizeInBytes = m_sbtEntrySize;
    dispatchDesc.HitGroupTable.StrideInBytes = m_sbtEntrySize;
    
    dispatchDesc.MissShaderTable.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress() + m_sbtEntrySize;
    dispatchDesc.MissShaderTable.SizeInBytes = m_sbtEntrySize;
    dispatchDesc.MissShaderTable.StrideInBytes = m_sbtEntrySize;
    
    dispatchDesc.RayGenerationShaderRecord.StartAddress = m_shaderBindingTable->GetGPUVirtualAddress();
    dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_sbtEntrySize;

    dispatchDesc.Width = m_width;
    dispatchDesc.Height = m_height;
    dispatchDesc.Depth = 1;

    m_commandList->DispatchRays(&dispatchDesc);

    // Transition UAV to COPY_SOURCE and Swapchain to COPY_DEST
    D3D12_RESOURCE_BARRIER copyBarriers[2] = {};
    copyBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    copyBarriers[0].Transition.pResource = m_raytracingOutput.Get();
    copyBarriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    copyBarriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    
    copyBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    copyBarriers[1].Transition.pResource = m_renderTargets[m_frameIndex].Get();
    copyBarriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    copyBarriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
    
    m_commandList->ResourceBarrier(2, copyBarriers);

    m_commandList->CopyResource(m_renderTargets[m_frameIndex].Get(), m_raytracingOutput.Get());

    // Transition Swapchain to PRESENT
    D3D12_RESOURCE_BARRIER presentBarrier = {};
    presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    presentBarrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
    presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    
    m_commandList->ResourceBarrier(1, &presentBarrier);

    ThrowIfFailed(m_commandList->Close());
    
    ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
    m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

    ThrowIfFailed(m_swapChain->Present(1, 0));

    FlushCommandQueue();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}

void DX12Renderer::FlushCommandQueue()
{
    const UINT64 fenceToWaitFor = m_fenceValue;
    ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fenceToWaitFor));
    m_fenceValue++;

    if (m_fence->GetCompletedValue() < fenceToWaitFor)
    {
        ThrowIfFailed(m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }
}
