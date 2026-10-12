# CustomEngine: DirectX 12 & DXR Path Tracer Engine

Welcome to the **CustomEngine** codebase. This project is a modern, high-performance C++20 3D game engine and path tracer built directly on the low-level **DirectX 12** API and **DirectX Raytracing (DXR / Tier 1.1)**.

---

## 1. Project Overview & Architecture

### Core Tech Stack
- **Language**: C++20 (MSVC `/W4 /MP /Zc:__cplusplus`)
- **Graphics API**: DirectX 12 Core (`d3d12.h`, `dxgi1_6.h`, `ID3D12Device5`, `ID3D12GraphicsCommandList4`)
- **Raytracing**: DirectX Raytracing (DXR) Tier 1.1, D3D12 State Objects, Shader Binding Tables (SBT)
- **Math Library**: `DirectXMath` (`DirectX::XMFLOAT3`, `DirectX::XMMATRIX`, etc.)
- **Shader Compiler**: DirectX Shader Compiler (`dxcompiler.dll`, `dxcapi.h`, Shader Model 6.5+)
- **Windowing & OS**: Win32 API (`Windows.h`), Message pump, high-resolution timer (`QueryPerformanceCounter`)
- **Asset Loading**: `tiny_obj_loader.h` for Wavefront OBJ parsing
- **Testing & Verification**: Standalone automated test suite (`EngineTests.exe`) & Headless Smoke Test runner (`--smoke-test`)

### Directory Layout
```text
CustomEngine/
├── AGENTS.md                  # Project overview, architecture, and AI agent instructions (this file)
├── .agents/
│   ├── rules/                 # Coding and graphics safety rules
│   │   ├── dx12-safety.md             # Resource barriers, memory lifetimes, synchronization
│   │   ├── dxr-raytracing.md          # AS building, SBT alignments, raytracing pipelines
│   │   ├── cpp20-graphics-style.md    # C++20 conventions, ComPtr, DirectXMath alignment
│   │   ├── hlsl-shader.md             # SM 6.5+ HLSL conventions and register mappings
│   │   └── loop-engineering-policy.md # Autonomous loop verification & test-first invariants
│   └── skills/                # Task-oriented development workflows
│       ├── loop-engineering/  # Automated inner dev loop (Build -> Unit Tests -> Smoke Test)
│       ├── build-and-run/     # Building and executing with debug layer monitoring
│       ├── compile-hlsl/      # Offline DXC shader validation and bytecode checking
│       ├── add-render-pass/   # Step-by-step checklist for adding rendering passes
│       └── dx12-troubleshoot/ # Diagnosis for Device Removed, TDR, and barrier bugs
├── src/
│   ├── main.cpp               # Win32 entry point (wWinMain) & main rendering loop (supports --smoke-test)
│   ├── Window.h / .cpp        # Win32 window creation and message dispatching
│   ├── DX12Renderer.h / .cpp  # Core DX12 & DXR engine, pipeline, AS, SBT, Render/Update
│   ├── Camera.h / .cpp        # 3D Camera, view/projection matrix calculations
│   ├── ShaderCompiler.h / .cpp# Runtime DXC shader compilation wrapper
│   └── tiny_obj_loader.h      # Single-header OBJ model loader
├── tests/
│   └── TestMain.cpp           # Automated C++20 test suite (Math, Shaders, D3D12/DXR, SBT)
├── shaders/
│   ├── raytracing.hlsl        # DXR Path Tracer (RayGen, Miss, ClosestHit, PCG RNG, accumulation)
│   └── shaders.hlsl           # Rasterization vertex & pixel shaders
├── assets/                    # 3D models (.obj) and textures
├── scripts/
│   └── run-dev-loop.ps1       # One-line autonomous loop verification script (~3s)
└── CMakeLists.txt             # CMake project definition, test targets & DLL auto-copy
```

---

## 2. Key Modules & Responsibilities

- **`main.cpp`**: Initializes `Window` and `DX12Renderer`. Computes delta time with `QueryPerformanceCounter` and drives the rendering loop. Supports `--smoke-test` argument for non-blocking headless execution.
- **`Window`**: Encapsulates Win32 window class registration, `CreateWindowExW`, window procedure (`WndProc`), and message pumping (`ProcessMessages`).
- **`DX12Renderer`**:
  - Initializes `IDXGIFactory4`, hardware `ID3D12Device5` (auto-detecting DXR-capable adapter, e.g. RTX 4060), Command Queue, Allocator, Command List.
  - Manages swap chains and double buffering (`FrameCount = 2`).
  - Manages Descriptor Heaps: `m_rtvHeap` and `m_srvUavHeap`.
  - Builds DXR Bottom-Level Acceleration Structure (BLAS) and Top-Level Acceleration Structure (TLAS).
  - Creates the DXR State Object (RTPSO) matching `RayPayload` size (40 bytes) and constructs the Shader Binding Table (SBT).
  - Handles Monte Carlo accumulation and tonemapping across successive frames.
- **`ShaderCompiler`**: Dynamically compiles HLSL files into DXIL blobs using `dxcompiler.dll` and `dxil.dll`.
- **`Camera`**: Computes view and projection matrices and their inverses for primary ray generation.
- **`tests/TestMain.cpp`**: Fast automated unit tests verifying camera math, DXC shader compilation, D3D12 device feature levels, and SBT alignments.

---

## 3. Autonomous Loop Engineering Workflow (Inner Feedback Loop)

In low-level DirectX 12 development, manual GUI testing is slow and prone to overlooking silent errors. **AI agents must always run the inner loop after making code or shader changes**:

### The One-Command Verification Loop
```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-dev-loop.ps1
```

This single command executes in ~3.5 seconds and verifies:
1. **Compilation**: Incremental build of `EngineTests.exe` and `CustomEngine.exe` with MSVC `/W4`.
2. **Unit Tests (`EngineTests.exe`)**: 4 automated tests (Camera math, DXC compilation, D3D12 device features, SBT alignments).
3. **Headless Engine Smoke Test (`CustomEngine.exe --smoke-test`)**: Runs 5 frames of the full DXR raytracing engine loop on the GPU and ensures clean exit code 0.

> [!IMPORTANT]
> **Hard Rule for AI Agents**: Never mark a task as completed without running `scripts\run-dev-loop.ps1` and confirming 100% green output.

---

## 4. Fundamental Engineering Guidelines for AI Agents

1. **Safety First in DX12**:
   - Never omit Resource Barriers. Transition resources explicitly before and after usage (e.g., `RENDER_TARGET` $\leftrightarrow$ `PRESENT`, `UNORDERED_ACCESS` $\leftrightarrow$ `COPY_SOURCE`).
   - Keep Upload Buffers alive until the GPU has executed the copy command (synchronize via `Fence`).
   - Consult `.agents/rules/dx12-safety.md`.

2. **Strict DXR Invariants**:
   - `MaxPayloadSizeInBytes` in `D3D12_RAYTRACING_SHADER_CONFIG` must match the exact byte size of `RayPayload` in HLSL (currently 40 bytes: `float3 color (12) + float hitT (4) + float3 normal (12) + float3 albedo (12)`).
   - Shader records in the SBT must be aligned to 32 bytes (`D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT`).
   - Shader tables in the SBT must start at 64-byte aligned addresses (`D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT`).
   - Consult `.agents/rules/dxr-raytracing.md`.

3. **C++20 & DirectX Conventions**:
   - Always use `Microsoft::WRL::ComPtr` for D3D12 interfaces.
   - Always wrap D3D calls with `ThrowIfFailed(HRESULT)`.
   - Store vectors and matrices as `DirectX::XMFLOAT3`, `DirectX::XMFLOAT4`, or `DirectX::XMFLOAT4X4` in class structures to maintain strict alignment; load into `DirectX::XMVECTOR` / `DirectX::XMMATRIX` only for calculations.
   - Consult `.agents/rules/cpp20-graphics-style.md`.

4. **Shader Conventions**:
   - Write HLSL for Shader Model 6.5+.
   - Keep register spaces and indices organized (`b0` for Camera CBV, `t0` for AS, `t1` for VertexBuffer, `t2` for Indices, `u0` for RenderTarget, `u1` for AccumulationTarget).
   - Consult `.agents/rules/hlsl-shader.md`.

5. **Utilizing Skills**:
   - Use the `loop-engineering` skill after every code modification.
   - Use the `compile-hlsl` skill before editing shaders to prevent runtime DXC crashes.
   - Use the `add-render-pass` skill when adding new rasterization, compute, or raytracing passes.
   - Use the `dx12-troubleshoot` skill when encountering TDR or `DXGI_ERROR_DEVICE_REMOVED`.
