---
name: dx12-troubleshoot
description: >-
  Use this skill when diagnosing DirectX 12 crashes, TDR (Timeout Detection and Recovery), DXGI_ERROR_DEVICE_REMOVED, black screens, or visual corruption.
---

# DirectX 12 Crash & TDR Troubleshooting Guide

This skill details diagnostic workflows for resolving GPU hangs, device removal, and rendering artifacts in CustomEngine.

---

## 1. Analyzing `DXGI_ERROR_DEVICE_REMOVED`

When a DirectX 12 call fails with `0x887A0005` (`DXGI_ERROR_DEVICE_REMOVED`), the root cause is almost always on the GPU. Inspect the removal reason immediately:

```cpp
HRESULT reason = m_device->GetDeviceRemovedReason();
// Common return codes:
// 0x887A0006 - DXGI_ERROR_DEVICE_HUNG (TDR: Shader infinite loop or memory stall)
// 0x887A0007 - DXGI_ERROR_DEVICE_RESET (Driver crashed and restarted)
// 0x887A002B - DXGI_ERROR_DRIVER_INTERNAL_ERROR
// 0x887A0001 - DXGI_ERROR_INVALID_CALL (Invalid arguments passed to API)
```

---

## 2. Enabling Advanced Diagnostics

### 1. D3D12 Debug Layer & Break-on-Severity
Ensure the D3D12 Debug Layer is active before creating the device:
```cpp
#if defined(_DEBUG)
ComPtr<ID3D12Debug> debugController;
if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
{
    debugController->EnableDebugLayer();
}
#endif
```

To break immediately into the debugger on errors, configure `ID3D12InfoQueue`:
```cpp
ComPtr<ID3D12InfoQueue> infoQueue;
if (SUCCEEDED(m_device.As(&infoQueue)))
{
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
}
```

### 2. GPU-Based Validation (GBV)
If a crash occurs without debug messages, enable GBV to catch out-of-bounds shader accesses and invalid descriptor indexing:
```cpp
ComPtr<ID3D12Debug1> debug1;
if (SUCCEEDED(debugController.As(&debug1)))
{
    debug1->SetEnableGPUBasedValidation(TRUE);
}
```

### 3. DRED (Device Removed Extended Data)
For post-mortem page fault and crash analysis:
```cpp
ComPtr<ID3D12DeviceRemovedExtendedDataSettings> dredSettings;
if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&dredSettings))))
{
    dredSettings->SetAutoBreadcrumbsEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
    dredSettings->SetPageFaultEnablement(D3D12_DRED_ENABLEMENT_FORCED_ON);
}
```

---

## 3. Top 5 Bug Patterns & Verification Checklist

| Symptom | Root Cause | Check / Fix |
| :--- | :--- | :--- |
| **GPU Hang (TDR)** | Infinite loop in ray tracing (`raytracing.hlsl`) | Verify ray bounce loop limit (`bounce < 4`) and that rays hitting nothing exit (`if (payload.hitT < 0.0f) break;`). |
| **TDR on TLAS build** | Missing UAV barrier on BLAS | Ensure a UAV barrier is recorded on `m_bottomLevelAS` before `BuildRaytracingAccelerationStructure` is called for TLAS. |
| **Silent Device Removal** | Upload Buffer prematurely destroyed | Check that temporary upload buffers (`vertexBufferUpload`, etc.) remain in memory until the command queue fence is signaled. |
| **Crash in DispatchRays** | SBT misalignment | Verify that RayGen, Miss, and HitGroup record strides are aligned to 32 bytes and table starts are aligned to 64 bytes. |
| **Black Screen / Flicker** | Missing Resource Barrier | Verify backbuffer transitions: `PRESENT` $\to$ `RENDER_TARGET` before rendering, and `RENDER_TARGET` $\to$ `PRESENT` before calling `m_swapChain->Present()`. |
