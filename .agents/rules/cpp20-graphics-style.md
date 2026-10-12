# Modern C++20 & Graphics Engine Coding Guidelines

This project leverages modern C++20 standards with MSVC (`/std:c++20 /W4 /MP /Zc:__cplusplus`). Follow these conventions for consistency, safety, and optimal SIMD vectorization.

---

## 1. Microsoft::WRL::ComPtr Usage

Always manage DirectX 12 COM objects using `Microsoft::WRL::ComPtr<T>`.

1. **Output Parameters (`&` operator vs `GetAddressOf`)**:
   - When passing a `ComPtr` to a function that initializes it (e.g. `D3D12CreateDevice`, `CreateCommittedResource`), use `.ReleaseAndGetAddressOf()` if re-initializing an existing member, or `&comPtr` if newly declared:
     ```cpp
     ThrowIfFailed(D3D12CreateDevice(
         adapter.Get(),
         D3D_FEATURE_LEVEL_12_1,
         IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf())
     ));
     ```
   - *Never* call `.Get()->Release()` manually. Let RAII handle reference counting.

2. **Passing Interfaces to APIs**:
   - Pass the raw pointer to D3D12 calls using `.Get()`:
     ```cpp
     m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
     ```

3. **QueryInterface Navigation**:
   - Use `.As()` instead of manual `QueryInterface`:
     ```cpp
     ComPtr<ID3D12Device5> device5;
     ThrowIfFailed(m_device.As(&device5));
     ```

---

## 2. DirectXMath SIMD Alignment Rules

`DirectXMath` utilizes SSE/AVX CPU vector instructions. Adhere strictly to its storage and load/store conventions to avoid alignment faults:

1. **Storage Types in Classes & Structs**:
   - Always store vector and matrix data in class members and structs using non-SIMD storage types:
     - `DirectX::XMFLOAT2`
     - `DirectX::XMFLOAT3`
     - `DirectX::XMFLOAT4`
     - `DirectX::XMFLOAT4X4`
   - **Do not** put `XMVECTOR` or `XMMATRIX` directly as member variables in dynamically allocated classes without explicit `alignas(16)` and aligned allocation overrides.

2. **SIMD Calculations**:
   - Load storage types into SIMD registers, perform math, and store back:
     ```cpp
     XMVECTOR pos = XMLoadFloat3(&m_position);
     XMVECTOR dir = XMLoadFloat3(&m_forward);
     pos = XMVectorMultiplyAdd(dir, XMVectorReplicate(speed * deltaTime), pos);
     XMStoreFloat3(&m_position, pos);
     ```

3. **Matrix Multiplication Order**:
   - DirectXMath uses **row-major** vectors:
     $$\text{v}_{\text{transformed}} = \text{v} \cdot \mathbf{W} \cdot \mathbf{V} \cdot \mathbf{P}$$
   - When multiplying matrices: `XMMatrixMultiply(world, XMMatrixMultiply(view, proj))` or `world * view * proj`.
   - When uploading to HLSL constant buffers, transpose if the shader assumes column-major (default in HLSL) or compile HLSL with `/Zpr` (row-major). In this engine, `XMMatrixTranspose` is applied before copying to CBV.

---

## 3. C++20 Engine Idioms

1. **Designated Initializers**:
   - Use designated initializers for clear and robust D3D12 descriptor structs:
     ```cpp
     D3D12_HEAP_PROPERTIES heapProps = {
         .Type = D3D12_HEAP_TYPE_DEFAULT,
         .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
         .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
         .CreationNodeMask = 1,
         .VisibleNodeMask = 1
     };
     ```

2. **Safe Spans over Raw Pointers**:
   - When passing array slices or vertex/index buffers to functions, prefer `std::span<const Vertex>` over `const Vertex*` and `size_t`.

3. **Error Handling**:
   - Every COM function returning `HRESULT` must be verified with `ThrowIfFailed(hr)`:
     ```cpp
     inline void ThrowIfFailed(HRESULT hr)
     {
         if (FAILED(hr))
         {
             throw std::runtime_error("DirectX 12 API failed.");
         }
     }
     ```
