# HLSL Shader Authoring & SM 6.5+ Standards

All shaders in this engine are compiled with the modern DirectX Shader Compiler (`dxcompiler.dll` / DXC) targeting Shader Model 6.5 or higher.

---

## 1. Register Spaces & Resource Binding Conventions

To avoid descriptor table conflicts and maintain clear Root Signature parity, adhere to the following register assignments:

| Register Type | Resource | Prefix / Usage | Example |
| :--- | :--- | :--- | :--- |
| `b0` .. `bN` | Constant Buffers | Per-frame/per-camera buffers | `cbuffer CameraCB : register(b0)` |
| `t0` | Acceleration Structure | Top-Level AS (DXR) | `RaytracingAccelerationStructure Scene : register(t0);` |
| `t1` .. `tN` | Structured Buffers / SRVs | Geometry & textures | `StructuredBuffer<Vertex> VertexBuffer : register(t1);` |
| `u0` .. `uN` | Unordered Access Views | Writeable output textures | `RWTexture2D<float4> RenderTarget : register(u0);` |
| `s0` .. `sN` | Samplers | Texture sampling states | `SamplerState LinearSampler : register(s0);` |

---

## 2. Constant Buffer 16-Byte Packing Rules

HLSL packs variables into 4-component 32-bit (16-byte) vectors:

1. **Avoid Stride Splitting**:
   - Variables cannot straddle 16-byte boundaries.
   - Example C++ and HLSL parity:
     ```hlsl
     // HLSL
     cbuffer CameraCB : register(b0)
     {
         float4x4 viewInverse; // 64 bytes (offset 0)
         float4x4 projInverse; // 64 bytes (offset 64)
         uint frameCount;      // 4 bytes  (offset 128)
         float3 padding;       // 12 bytes (offset 132) -> completes 144 bytes
     };
     ```
     ```cpp
     // C++
     struct ConstantBuffer
     {
         XMMATRIX viewInverse;
         XMMATRIX projInverse;
         UINT frameCount;
         float padding[3];
     };
     ```

2. **256-Byte D3D12 CBV Alignment**:
   - The overall byte size of constant buffer resources in D3D12 must be a multiple of 256 bytes:
     $$\text{Size}_{\text{aligned}} = (\text{sizeof}(T) + 255) \ \& \ \sim 255$$

---

## 3. DXR Shader Functions & Semantics

When writing raytracing shaders (`shaders/raytracing.hlsl`):

1. **Shader Entry Attributes**:
   - Use HLSL SM 6.x function attributes:
     - `[shader("raygeneration")]` for primary ray dispatch
     - `[shader("miss")]` for sky/background lighting
     - `[shader("closesthit")]` for material shading and secondary ray recursion
     - `[shader("anyhit")]` for alpha testing (if implemented)

2. **Payload Match**:
   - The `RayPayload` struct passed to `TraceRay` must match the `MaxPayloadSizeInBytes` in C++ `DX12Renderer::CreateRaytracingPipeline`.
   ```hlsl
   struct RayPayload
   {
       float3 color;  // 12 bytes
       float hitT;    // 4 bytes
       float3 normal; // 12 bytes
       float3 albedo; // 12 bytes
   }; // Total = 40 bytes
   ```

3. **Built-in Intrinsics**:
   - `DispatchRaysIndex().xy`: Current pixel coordinates being evaluated.
   - `DispatchRaysDimensions().xy`: Total resolution of the dispatch.
   - `WorldRayDirection()` and `WorldRayOrigin()`: Active ray in world coordinates.
   - `RayTCurrent()`: Distance from ray origin to current intersection point.
   - `PrimitiveIndex()`: Index of the intersected triangle in the vertex/index buffer.
