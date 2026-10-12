---
name: compile-hlsl
description: >-
  Use this skill when modifying, adding, or debugging HLSL shaders (e.g. raytracing.hlsl, shaders.hlsl) to perform static syntax checking and offline compilation with DXC.
---

# HLSL Compilation & Validation Skill

This runbook instructs the agent on how to validate HLSL syntax, verify reflection, and check DXIL bytecode compatibility using the DirectX Shader Compiler (`dxc.exe`) without launching the full engine runtime.

---

## 1. DXC Executable Location

On Windows development environments with the Windows 10/11 SDK installed, `dxc.exe` is located at:
```text
C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe
```
*(Or the latest version under `C:\Program Files (x86)\Windows Kits\10\bin\*\x64\dxc.exe`)*

---

## 2. Compilation Profiles & Commands

### 1. DXR Raytracing Shaders (`lib_6_5`)
Raytracing shaders contain multiple entry points (`[shader("raygeneration")]`, `[shader("miss")]`, `[shader("closesthit")]`) and must be compiled as a library (`lib_6_5` or `lib_6_6`):
```powershell
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe" -T lib_6_5 shaders\raytracing.hlsl
```
**Verification Points**:
- Check that `RayPayload` byte size matches C++ `MaxPayloadSizeInBytes` (e.g., 40 bytes).
- Verify resource bindings: `Scene` (t0), `VertexBuffer` (t1), `Indices` (t2), `RenderTarget` (u0), `AccumulationTarget` (u1), `CameraCB` (b0).

### 2. Vertex Shaders (`vs_6_5`)
```powershell
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe" -T vs_6_5 -E VSMain shaders\shaders.hlsl
```

### 3. Pixel Shaders (`ps_6_5`)
```powershell
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe" -T ps_6_5 -E PSMain shaders\shaders.hlsl
```

### 4. Compute Shaders (`cs_6_5`)
```powershell
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\dxc.exe" -T cs_6_5 -E CSMain shaders\my_compute.hlsl
```

---

## 3. Pre-Flight Validation Checklist

Before testing shaders in the engine:
1. **Compilation Return Code**: Ensure DXC returns exit code `0` with no syntax errors.
2. **Warning Analysis**: Watch for implicit truncation warnings, matrix transposition warnings, or uninitialized variables.
3. **Synchronization with Binary Folder**: After modifying shaders in `shaders/`, remember to copy them to the binary directory:
   ```powershell
   Copy-Item shaders\* build\bin\Debug\shaders\ -Force
   ```
