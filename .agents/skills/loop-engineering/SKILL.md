---
name: loop-engineering
description: >-
  Use this skill whenever making changes to engine code, adding new rendering passes, modifying shaders, or refactoring to execute the autonomous inner development loop (Build -> Unit Test -> DXR Smoke Test -> Diagnose -> Iterate).
---

# Autonomous Inner Loop Engineering Workflow

This skill defines the inner development and verification loop for `CustomEngine`. In low-level DirectX 12 and DXR development, running manual GUI tests is error-prone and slow. This feedback loop guarantees rapid, automated verification in ~3 seconds.

---

## 1. The 5-Phase Development Loop

```mermaid
flowchart LR
    A["1. Code / Shader Edit"] --> B["2. Fast Offline Check<br/>(compile-hlsl)"]
    B --> C["3. Run Dev Loop Script<br/>(scripts/run-dev-loop.ps1)"]
    C -->|Failure| D["4. Diagnose Failure<br/>(dx12-troubleshoot)"]
    D --> A
    C -->|Pass (100% Green)| E["5. Task Completed"]
```

---

## 2. One-Command Inner Loop Execution

After every modification to `src/` or `shaders/`, execute the feedback loop script from the project root:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-dev-loop.ps1
```

### What this script verifies automatically:
1. **Incremental Build**: Compiles both `EngineTests.exe` and `CustomEngine.exe` with MSVC `/W4` under `Debug`.
2. **Automated Unit & Feature Tests (`EngineTests.exe`)**:
   - `Test_CameraMath`: Vector orthogonality, view/projection matrix inversion.
   - `Test_ShaderCompilation`: DXC runtime compilation of `shaders/raytracing.hlsl` and `shaders/shaders.hlsl`.
   - `Test_DX12DeviceAndDXRFeatures`: Hardware adapter selection, D3D12 Device5 creation, DXR Tier 1.1 support check, command allocator/list lifecycle.
   - `Test_SBTAlignmentInvariants`: 32-byte record and 64-byte table alignment calculations.
3. **Headless DXR Smoke Test (`CustomEngine.exe --smoke-test`)**:
   - Spawns the engine, selects the DXR-capable GPU (e.g. RTX 4060), builds BLAS & TLAS, constructs the SBT, dispatches 5 frames of path-traced rays with accumulation and tonemapping, and exits cleanly with code `0`.

---

## 3. Failure Diagnosis & Iteration Procedure

If `run-dev-loop.ps1` fails:

### Case A: Build Failure (Stage 1)
- Inspect compiler errors (missing headers, SIMD type mismatch, missing `ComPtr`).
- Check C++20 conventions in `.agents/rules/cpp20-graphics-style.md`.

### Case B: Unit Test Failure (Stage 2)
- Check which `[ RUN ] ... [ FAILED ]` test failed in the console output.
- For shader failures: run `& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe" -T lib_6_5 shaders\raytracing.hlsl` to inspect HLSL syntax errors.

### Case C: Smoke Test Failure (Stage 3)
- Check `smoke_error.txt` generated in the project root.
- Common causes:
  - `HRESULT 0x80070057` (`E_INVALIDARG`): Mismatch in `MaxPayloadSizeInBytes` (must be 40 bytes) or invalid root signature / state object subobject.
  - `HRESULT 0x887A0005` (`DXGI_ERROR_DEVICE_REMOVED`): GPU TDR or barrier violation. Follow `.agents/skills/dx12-troubleshoot/SKILL.md`.

---

## 4. Expanding the Test Suite for New Features

When adding new rendering techniques (e.g. compute post-processing, G-Buffer, shadows):
1. Add corresponding unit test assertions in `tests/TestMain.cpp`.
2. Ensure `scripts/run-dev-loop.ps1` runs clean before declaring the feature complete.
