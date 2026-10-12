# DXR (DirectX Raytracing) Safety & Architecture Rules

This engine implements DXR (Tier 1.1) Raytracing and Path Tracing. Follow these strict rules to prevent GPU hangs, invalid dispatch errors, and visual anomalies.

---

## 1. Acceleration Structure (BLAS & TLAS) Invariants

1. **Buffer Creation Requirements**:
   - Both Scratch buffers and Result buffers for AS must be created on `D3D12_HEAP_TYPE_DEFAULT` with:
     ```cpp
     D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
     ```
   - The initial resource state for AS buffers must be `D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE`.
   - The initial resource state for scratch buffers must be `D3D12_RESOURCE_STATE_COMMON` or `D3D12_RESOURCE_STATE_UNORDERED_ACCESS`.

2. **Alignment & Scratch Size**:
   - Acceleration structure GPU virtual addresses must be aligned to 256 bytes (`D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT`).
   - If reusing a single scratch buffer for both BLAS and TLAS builds, its size must be:
     ```cpp
     UINT64 scratchBufferSize = (std::max)(blasPrebuildInfo.ScratchDataSizeInBytes, tlasPrebuildInfo.ScratchDataSizeInBytes);
     ```

3. **UAV Barrier Between BLAS and TLAS**:
   - TLAS generation depends on BLAS completion. You **must** insert a UAV barrier on the BLAS buffer before recording the TLAS build command:
     ```cpp
     D3D12_RESOURCE_BARRIER uavBarrier = {};
     uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
     uavBarrier.UAV.pResource = m_bottomLevelAS.Get();
     m_commandList->ResourceBarrier(1, &uavBarrier);
     ```

---

## 2. Shader Binding Table (SBT) Alignment Rules

The Shader Binding Table is one of the most common sources of DXR validation crashes. Adhere strictly to the hardware alignment constraints:

1. **Alignment Constants**:
   - **Shader Record Alignment**: 32 bytes (`D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT`).
   - **Shader Table Alignment**: 64 bytes (`D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT`).

2. **Record & Table Size Calculation Pattern**:
   ```cpp
   // Helper alignment lambda
   auto Align = [](UINT size, UINT alignment) {
       return (size + (alignment - 1)) & ~(alignment - 1);
   };

   // 1. RayGen Record (Exactly 1 record)
   UINT rayGenRecordSize = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + localArgSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
   UINT rayGenTableSize = Align(rayGenRecordSize, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

   // 2. Miss Table
   UINT missRecordSize = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
   UINT missTableSize = Align(missRecordSize * numMissShaders, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);

   // 3. HitGroup Table
   UINT hitRecordSize = Align(D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT);
   UINT hitTableSize = Align(hitRecordSize * numHitGroups, D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT);
   ```

3. **Writing to the SBT Upload Buffer**:
   - Use `ID3D12StateObjectProperties::GetShaderIdentifier(L"ShaderName")` to retrieve the 32-byte identifier.
   - Copy identifiers with `memcpy` into the mapped SBT pointer at calculated offsets.

---

## 3. Pipeline State Object (RTPSO) Configuration

1. **Payload & Attribute Size Matching**:
   - In `D3D12_RAYTRACING_SHADER_CONFIG`:
     - `MaxPayloadSizeInBytes`: Must equal `sizeof(RayPayload)` in HLSL.
       - e.g., `float3 color (12) + float hitT (4) + float3 normal (12) + float3 albedo (12) = 40 bytes`.
     - `MaxAttributeSizeInBytes`: Must be `sizeof(XMFLOAT2)` = 8 bytes for built-in triangle attributes (`BuiltInTriangleIntersectionAttributes`).
   - Underestimating these sizes causes memory corruption or driver crashes.

2. **Recursion Depth Optimization**:
   - For iterative path tracing (where bounces are processed in a loop inside `RayGen`), set `MaxTraceRecursionDepth = 1`.
   - Setting high recursion depth wastes excessive GPU stack memory.

---

## 4. DispatchRays Execution

When dispatching rays via `m_commandList->DispatchRays(&dispatchDesc)`:
- `dispatchDesc.RayGenerationShaderRecord.StartAddress` must point to the RayGen entry.
- `dispatchDesc.RayGenerationShaderRecord.SizeInBytes` must equal `rayGenRecordSize`.
- `dispatchDesc.MissShaderTable.StrideInBytes` must equal `missRecordSize`.
- `dispatchDesc.HitGroupTable.StrideInBytes` must equal `hitRecordSize`.
- Ensure output textures (`RenderTarget`, `AccumulationTarget`) are transitioned to `D3D12_RESOURCE_STATE_UNORDERED_ACCESS` prior to dispatch.
