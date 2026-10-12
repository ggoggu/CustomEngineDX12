---
name: build-and-run
description: >-
  Use this skill when the user asks to build the DirectX 12 CustomEngine, compile the project with CMake and MSVC, or run the executable to test changes and inspect debug layer messages.
---

# Build and Run Workflow for CustomEngine

This runbook guides the agent through compiling, validating asset deployment, and executing the DirectX 12 Path Tracer engine.

---

## 1. Prerequisites & Environment Check

- **Working Directory**: Project root (`CustomEngine/`)
- **Generator**: MSVC / Visual Studio 2022 (64-bit)
- **Dependencies**: Windows SDK 10.0.19041+ (contains `d3d12.lib`, `dxgi.lib`, `dxcompiler.lib`)

---

## 2. Step-by-Step Execution

### Step 1: Configure CMake (if `build/` is missing or `CMakeLists.txt` changed)
```powershell
cmake -B build -S .
```

### Step 2: Build the Debug Binary
```powershell
cmake --build build --config Debug
```
- Verify MSBuild outputs `CustomEngine.vcxproj -> ... CustomEngine.exe` with exit code `0`.
- If compilation fails with syntax errors, inspect the reported file and line numbers, fix the issue, and re-run.

### Step 3: Verify Asset and Shader Deployment
CMake's `POST_BUILD` step automatically copies folders. Confirm that target directories exist:
```powershell
Test-Path build\bin\Debug\shaders\raytracing.hlsl
Test-Path build\bin\Debug\assets
```
If shaders were edited and need immediate replacement in the target binary folder without full rebuild:
```powershell
Copy-Item -Recurse -Force shaders build\bin\Debug\
```

### Step 4: Run the Engine
Run the engine binary:
```powershell
.\build\bin\Debug\CustomEngine.exe
```

---

## 3. Verifying Results & DirectX Debug Layer

When running in `Debug` configuration:
1. Ensure no crash dialog appears.
2. Check for D3D12 Debug Layer messages in the standard output or Visual Studio output window.
3. If the window opens and the path tracer renders accumulation frames, the build and run is verified.
