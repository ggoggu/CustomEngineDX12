# Loop Engineering & Automated Verification Policy

This project strictly adheres to **autonomous loop engineering**. In low-level DirectX 12 graphics programming, unverified code changes cause silent memory corruption, GPU hangs (TDR), or initialization deadlocks.

---

## 1. Mandatory Inner Feedback Loop

After modifying any file in `src/`, `shaders/`, or `CMakeLists.txt`, the agent **must** execute the verification loop before concluding the turn or declaring the task finished:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-dev-loop.ps1
```

### The Invariants of a Passing Loop
1. **Zero Warnings/Errors in Build**: Compiles cleanly with MSVC `/W4 /MP /Zc:__cplusplus`.
2. **100% Pass in `EngineTests.exe`**: All mathematical, shader compilation, D3D12 device, and SBT alignment assertions must pass.
3. **Exit Code 0 in `CustomEngine.exe --smoke-test`**: The full DXR Path Tracer must initialize, execute 5 frames of ray dispatches with accumulation, and exit cleanly without throwing or crashing.

---

## 2. Bug Fixing & Regression Rule

- If a crash or test failure occurs, never guess or make speculative code changes.
- Follow the diagnostic pipeline:
  1. Inspect `smoke_error.txt` for the exact line and HRESULT code.
  2. Check D3D12 Debug Layer messages.
  3. Formulate a minimal, targeted fix.
  4. Re-run `scripts\run-dev-loop.ps1` to verify the fix and prevent regressions.

---

## 3. Test Coverage Requirement for New Engine Features

When introducing a new feature:
- **Math / Algorithms**: Add unit tests in `tests/TestMain.cpp`.
- **Shaders**: Add offline compilation checks to `Test_ShaderCompilation`.
- **State Objects & SBT**: Verify stride and table alignments in `Test_SBTAlignmentInvariants`.
