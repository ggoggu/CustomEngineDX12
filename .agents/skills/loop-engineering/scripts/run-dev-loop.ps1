# ==============================================================================
# CustomEngine Autonomous Inner Loop Engineering Script (run-dev-loop.ps1)
# ==============================================================================
param(
    [string]$Config = "Debug"
)

$ErrorActionPreference = "Stop"
$swTotal = [System.Diagnostics.Stopwatch]::StartNew()

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " [LOOP] Starting Autonomous Engine Development Loop ($Config)" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

# Stage 1: Build
Write-Host "`n>>> [Stage 1/3] Building Engine & Tests..." -ForegroundColor Yellow
$swBuild = [System.Diagnostics.Stopwatch]::StartNew()
cmake --build build --config $Config --target EngineTests CustomEngine
if ($LASTEXITCODE -ne 0) {
    Write-Host "`n[FAIL] Build failed with exit code $LASTEXITCODE" -ForegroundColor Red
    exit $LASTEXITCODE
}
$swBuild.Stop()
Write-Host "[OK] Build succeeded in $($swBuild.ElapsedMilliseconds)ms" -ForegroundColor Green

# Stage 2: Unit & Feature Tests
Write-Host "`n>>> [Stage 2/3] Running Engine Unit & Feature Tests..." -ForegroundColor Yellow
$testExe = ".\build\bin\$Config\EngineTests.exe"
if (-not (Test-Path $testExe)) {
    Write-Host "[FAIL] Test executable not found: $testExe" -ForegroundColor Red
    exit 1
}

$swTest = [System.Diagnostics.Stopwatch]::StartNew()
$pTest = Start-Process -FilePath $testExe -Wait -PassThru -NoNewWindow
$swTest.Stop()

if ($pTest.ExitCode -ne 0) {
    Write-Host "`n[FAIL] Unit tests failed with exit code $($pTest.ExitCode)" -ForegroundColor Red
    exit $pTest.ExitCode
}
Write-Host "[OK] All unit tests passed in $($swTest.ElapsedMilliseconds)ms" -ForegroundColor Green

# Stage 3: Headless Engine Smoke Test (5 Frames)
Write-Host "`n>>> [Stage 3/3] Running Headless Engine Smoke Test (DX12 DXR Pipeline)..." -ForegroundColor Yellow
$engineExe = ".\build\bin\$Config\CustomEngine.exe"
Remove-Item smoke_error.txt -ErrorAction SilentlyContinue

$swSmoke = [System.Diagnostics.Stopwatch]::StartNew()
$pSmoke = Start-Process -FilePath $engineExe -ArgumentList "--smoke-test" -Wait -PassThru -NoNewWindow
$swSmoke.Stop()

if ($pSmoke.ExitCode -ne 0) {
    Write-Host "`n[FAIL] Engine smoke test failed with exit code $($pSmoke.ExitCode)" -ForegroundColor Red
    if (Test-Path smoke_error.txt) {
        Write-Host "Error Details: $(Get-Content smoke_error.txt)" -ForegroundColor Red
    }
    exit $pSmoke.ExitCode
}
Write-Host "[OK] Engine smoke test (DXR Path Tracer) passed in $($swSmoke.ElapsedMilliseconds)ms" -ForegroundColor Green

# Summary
$swTotal.Stop()
Write-Host "`n==========================================================" -ForegroundColor Green
Write-Host " [SUCCESS] Engine Loop Verified 100% Green! Total: $($swTotal.ElapsedMilliseconds)ms" -ForegroundColor Green
Write-Host "==========================================================" -ForegroundColor Green
exit 0
