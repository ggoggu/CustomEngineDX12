---
name: add-render-pass
description: >-
  Use this skill when the user asks to add a new rendering pass, such as a rasterization pass (shadows, G-Buffer), a post-processing compute pass, or a raytracing secondary pass.
---

# Adding a New Render Pass in CustomEngine

This skill details the canonical procedure to implement and integrate a new rendering pass into `DX12Renderer`.

---

## 1. Pass Architectural Checklist

Every rendering pass in DirectX 12 requires 5 core components:

```text
[1. Root Signature] ──> [2. Pipeline State (PSO)] ──> [3. Descriptors & Buffers]
                                                                │
                                                                ▼
                                                [4. Resource Barriers & Commands]
```

---

## 2. Step-by-Step Implementation Guide

### Step 1: Define the Root Signature
- Plan resource registers (`register(b#)`, `register(t#)`, `register(u#)`).
- For Compute / Graphics passes:
  ```cpp
  CD3DX12_ROOT_PARAMETER rootParams[2];
  rootParams[0].InitAsConstantBufferView(0); // b0
  CD3DX12_DESCRIPTOR_RANGE uavRange;
  uavRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0
  rootParams[1].InitAsDescriptorTable(1, &uavRange);

  CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(2, rootParams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
  ComPtr<ID3DBlob> serializedRootSig;
  ComPtr<ID3DBlob> errorBlob;
  ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob));
  ThrowIfFailed(m_device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_newPassRootSig)));
  ```

### Step 2: Create the Pipeline State Object (PSO)
- **For Compute Pass**:
  ```cpp
  D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.pRootSignature = m_newPassRootSig.Get();
  psoDesc.CS = { csBlob->GetBufferPointer(), csBlob->GetBufferSize() };
  ThrowIfFailed(m_device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_newPassPSO)));
  ```
- **For Graphics Pass**:
  - Configure `InputLayout`, `RasterizerState`, `BlendState`, `DepthStencilState`, and `RTVFormats`.

### Step 3: Allocate Resources and Descriptor Handles
- Allocate target textures or buffers on `D3D12_HEAP_TYPE_DEFAULT`.
- Allocate descriptors in the appropriate heap:
  - Backbuffer targets: `m_rtvHeap`
  - Shader readable / writeable: `m_srvUavHeap`
- Compute CPU and GPU descriptor handles using `m_srvUavDescriptorSize`.

### Step 4: Record Commands in `Render()`
1. **Transition Resource Before Execution**:
   ```cpp
   D3D12_RESOURCE_BARRIER preBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
       m_targetResource.Get(),
       D3D12_RESOURCE_STATE_COMMON,
       D3D12_RESOURCE_STATE_UNORDERED_ACCESS
   );
   m_commandList->ResourceBarrier(1, &preBarrier);
   ```

2. **Bind Pipeline & Resources**:
   ```cpp
   m_commandList->SetComputeRootSignature(m_newPassRootSig.Get());
   m_commandList->SetPipelineState(m_newPassPSO.Get());
   ID3D12DescriptorHeap* heaps[] = { m_srvUavHeap.Get() };
   m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
   m_commandList->SetComputeRootDescriptorTable(1, targetUavGpuHandle);
   ```

3. **Dispatch / Draw**:
   ```cpp
   m_commandList->Dispatch((m_width + 15) / 16, (m_height + 15) / 16, 1);
   ```

4. **Synchronize or Transition Resource for Subsequent Passes**:
   - If written as UAV and consumed as SRV next: Transition to `D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE` or `NON_PIXEL_SHADER_RESOURCE`.
   - If consumed in another dispatch as UAV: Insert a UAV Barrier.
