# -*- coding: utf-8 -*-
"""
DirectX 12 & DXR CustomEngine Deep-Dive Guide Generator
Generates an all-in-one HTML guide with IDE-style syntax highlighting and compiles it to PDF using Headless Edge.
"""

import os
import sys
import subprocess
from pathlib import Path
import pygments
from pygments.lexers import get_lexer_by_name
from pygments.formatters import HtmlFormatter

WORKSPACE_DIR = Path(r"c:\Users\oogg\Desktop\CustomEngine")
REPORT_DIR = WORKSPACE_DIR / "Report"
HTML_OUTPUT = REPORT_DIR / "CustomEngine_DX12_DeepDive_Guide.html"
PDF_OUTPUT = REPORT_DIR / "CustomEngine_DX12_DeepDive_Guide.pdf"
EDGE_PATH = r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"

REPORT_DIR.mkdir(parents=True, exist_ok=True)

# Pygments HTML Formatter with dark theme
formatter = HtmlFormatter(style="monokai", nowrap=False, linenos=False)
pygments_css = formatter.get_style_defs('.highlight')

def highlight_code(code_str: str, lexer_type: str = "cpp", file_name: str = "") -> str:
    """Highlights code string with Pygments and wraps in an IDE-style container."""
    lexer = get_lexer_by_name(lexer_type)

    highlighted = pygments.highlight(code_str, lexer, formatter)
    
    header_html = f"""
    <div class="code-window">
        <div class="code-window-header">
            <div class="window-buttons">
                <span class="btn-close"></span>
                <span class="btn-min"></span>
                <span class="btn-max"></span>
            </div>
            <span class="code-file-title">{file_name}</span>
        </div>
        <div class="code-window-body">
            {highlighted}
        </div>
    </div>
    """
    return header_html

def read_file_safe(path: Path) -> str:
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        return f.read()

def get_code_slice(file_path: Path, start_line: int, end_line: int) -> str:
    content = read_file_safe(file_path)
    lines = content.splitlines()
    selected = lines[start_line - 1 : end_line]
    return "\n".join(selected)

def build_report():
    print("Reading engine source code files...")

    main_cpp = read_file_safe(WORKSPACE_DIR / "src" / "main.cpp")
    window_h = read_file_safe(WORKSPACE_DIR / "src" / "Window.h")
    window_cpp = read_file_safe(WORKSPACE_DIR / "src" / "Window.cpp")
    camera_h = read_file_safe(WORKSPACE_DIR / "src" / "Camera.h")
    camera_cpp = read_file_safe(WORKSPACE_DIR / "src" / "Camera.cpp")
    shader_compiler_h = read_file_safe(WORKSPACE_DIR / "src" / "ShaderCompiler.h")
    shader_compiler_cpp = read_file_safe(WORKSPACE_DIR / "src" / "ShaderCompiler.cpp")
    dx12_renderer_h = read_file_safe(WORKSPACE_DIR / "src" / "DX12Renderer.h")
    dx12_renderer_cpp = read_file_safe(WORKSPACE_DIR / "src" / "DX12Renderer.cpp")
    raytracing_hlsl = read_file_safe(WORKSPACE_DIR / "shaders" / "raytracing.hlsl")
    shaders_hlsl = read_file_safe(WORKSPACE_DIR / "shaders" / "shaders.hlsl")

    # Generate highlighted HTML blocks
    block_main_cpp = highlight_code(main_cpp, "cpp", "src/main.cpp")
    block_window_init = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "Window.cpp", 1, 45), "cpp", "src/Window.cpp (윈도우 초기화 & 등록)")
    block_window_proc = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "Window.cpp", 66, 93), "cpp", "src/Window.cpp (WindowProc 및 인스턴스 바인딩)")
    
    block_init_adapter = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 105, 148), "cpp", "src/DX12Renderer.cpp (디버그 레이어 & DXR 어댑터 탐색)")
    block_init_swapchain = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 158, 200), "cpp", "src/DX12Renderer.cpp (명령 큐, 스왑 체인 & RTV 힙)")
    
    block_camera_vec = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "Camera.cpp", 64, 78), "cpp", "src/Camera.cpp (UpdateVectors)")
    block_camera_phys = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "Camera.cpp", 43, 62), "cpp", "src/Camera.cpp (UpdatePhysics)")
    
    block_shader_compiler = highlight_code(shader_compiler_cpp, "cpp", "src/ShaderCompiler.cpp")
    
    block_default_buffer = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 48, 103), "cpp", "src/DX12Renderer.cpp (CreateDefaultBuffer)")
    block_load_model = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 581, 645), "cpp", "src/DX12Renderer.cpp (LoadModel)")
    
    block_blas = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 650, 673), "cpp", "src/DX12Renderer.cpp (BLAS 정의 및 Prebuild)")
    block_tlas = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 732, 788), "cpp", "src/DX12Renderer.cpp (TLAS 인스턴스 및 빌드)")
    
    block_root_sig = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 324, 385), "cpp", "src/DX12Renderer.cpp (글로벌 루트 시그니처)")
    block_rtpso = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 475, 531), "cpp", "src/DX12Renderer.cpp (RTPSO 5대 서브오브젝트)")
    
    block_sbt = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 533, 580), "cpp", "src/DX12Renderer.cpp (CreateShaderBindingTable)")
    
    block_hlsl_rng = highlight_code(get_code_slice(WORKSPACE_DIR / "shaders" / "raytracing.hlsl", 29, 60), "hlsl", "shaders/raytracing.hlsl (PCG 난수 & 반구 코사인 샘플링)")
    block_hlsl_raygen = highlight_code(get_code_slice(WORKSPACE_DIR / "shaders" / "raytracing.hlsl", 61, 128), "hlsl", "shaders/raytracing.hlsl (RayGen 4회 바운스 & 누적)")
    block_hlsl_hit = highlight_code(get_code_slice(WORKSPACE_DIR / "shaders" / "raytracing.hlsl", 130, 157), "hlsl", "shaders/raytracing.hlsl (Miss & ClosestHit)")
    
    block_render_update = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 790, 805), "cpp", "src/DX12Renderer.cpp (Update)")
    block_render_render = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 807, 896), "cpp", "src/DX12Renderer.cpp (Render & 리소스 배리어)")
    block_render_flush = highlight_code(get_code_slice(WORKSPACE_DIR / "src" / "DX12Renderer.cpp", 898, 910), "cpp", "src/DX12Renderer.cpp (FlushCommandQueue)")

    template_parts = []

    template_parts.append("""<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>CustomEngine DirectX 12 & DXR 패스 트레이서 코드 심층 해설서</title>
<style>
@page {
    size: A4;
    margin: 18mm 16mm 18mm 16mm;
    @bottom-right {
        content: counter(page);
        font-family: 'Segoe UI', 'Pretendard', sans-serif;
        font-size: 9pt;
        color: #888888;
    }
}

* {
    box-sizing: border-box;
}

body {
    font-family: 'Pretendard', 'Malgun Gothic', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
    font-size: 10pt;
    line-height: 1.65;
    color: #24292f;
    background-color: #ffffff;
    margin: 0;
    padding: 0;
}

h1, h2, h3, h4, h5 {
    color: #0f172a;
    font-weight: 700;
    line-height: 1.3;
    page-break-after: avoid;
}

h1.doc-title {
    font-size: 24pt;
    text-align: center;
    margin-top: 20px;
    margin-bottom: 6px;
    color: #0284c7;
    letter-spacing: -0.5px;
}

.doc-subtitle {
    font-size: 12pt;
    text-align: center;
    color: #475569;
    margin-bottom: 24px;
    font-weight: 500;
}

.meta-box {
    background: #f8fafc;
    border: 1px solid #e2e8f0;
    border-radius: 8px;
    padding: 12px 18px;
    margin-bottom: 30px;
    font-size: 9pt;
    color: #334155;
    display: flex;
    justify-content: space-around;
}

.meta-item strong {
    color: #0f172a;
}

h2.part-header {
    font-size: 15pt;
    border-bottom: 2.5px solid #0284c7;
    padding-bottom: 5px;
    margin-top: 32px;
    margin-bottom: 14px;
    color: #0f172a;
    display: flex;
    align-items: center;
}

.part-tag {
    background: #0284c7;
    color: white;
    font-size: 9.5pt;
    font-weight: bold;
    padding: 3px 9px;
    border-radius: 4px;
    margin-right: 10px;
}

h3.section-header {
    font-size: 12pt;
    color: #1e293b;
    margin-top: 20px;
    margin-bottom: 10px;
    border-left: 4px solid #0284c7;
    padding-left: 8px;
}

h4.subsection-header {
    font-size: 10.5pt;
    color: #334155;
    margin-top: 14px;
    margin-bottom: 6px;
}

p {
    margin-top: 0;
    margin-bottom: 10px;
    text-align: justify;
    word-break: keep-all;
}

ul, ol {
    margin-top: 0;
    margin-bottom: 12px;
    padding-left: 22px;
}

li {
    margin-bottom: 4px;
}

.code-window {
    background-color: #1e1e1e;
    border-radius: 8px;
    margin: 12px 0 16px 0;
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.12);
    overflow: hidden;
    border: 1px solid #333333;
    page-break-inside: avoid;
}

.code-window-header {
    background-color: #2d2d2d;
    padding: 6px 14px;
    display: flex;
    align-items: center;
    border-bottom: 1px solid #3e3e42;
}

.window-buttons {
    display: inline-flex;
    gap: 6px;
    margin-right: 12px;
}

.btn-close, .btn-min, .btn-max {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    display: inline-block;
}
.btn-close { background-color: #ff5f56; }
.btn-min { background-color: #ffbd2e; }
.btn-max { background-color: #27c93f; }

.code-file-title {
    color: #d4d4d4;
    font-family: 'Consolas', 'Cascadia Code', monospace;
    font-size: 8.5pt;
    font-weight: 600;
}

.code-window-body {
    padding: 10px 14px;
    overflow-x: auto;
}

.code-window-body pre {
    margin: 0;
    font-family: 'Consolas', 'Cascadia Code', 'Fira Code', 'Courier New', monospace;
    font-size: 8.5pt;
    line-height: 1.48;
    white-space: pre-wrap;
    word-break: break-all;
    color: #d4d4d4;
}

code {
    font-family: 'Consolas', 'Cascadia Code', monospace;
    font-size: 9pt;
    background: #f1f5f9;
    color: #0f172a;
    padding: 1px 5px;
    border-radius: 4px;
    border: 1px solid #e2e8f0;
}
""")

    template_parts.append(f"\n{pygments_css}\n.highlight {{ background: transparent !important; }}\n")

    template_parts.append("""
.callout {
    border-radius: 6px;
    padding: 10px 14px;
    margin: 12px 0;
    page-break-inside: avoid;
    font-size: 9.5pt;
}

.callout-info {
    background-color: #eff6ff;
    border-left: 4px solid #3b82f6;
    color: #1e3a8a;
}

.callout-tip {
    background-color: #ecfdf5;
    border-left: 4px solid #10b981;
    color: #065f46;
}

.callout-warning {
    background-color: #fffbeb;
    border-left: 4px solid #f59e0b;
    color: #92400e;
}

.callout-danger {
    background-color: #fef2f2;
    border-left: 4px solid #ef4444;
    color: #991b1b;
}

.callout strong {
    display: block;
    margin-bottom: 3px;
    font-size: 9.5pt;
}

table {
    width: 100%;
    border-collapse: collapse;
    margin: 12px 0;
    font-size: 9pt;
    page-break-inside: avoid;
}

th, td {
    border: 1px solid #cbd5e1;
    padding: 7px 9px;
    text-align: left;
}

th {
    background-color: #f1f5f9;
    color: #0f172a;
    font-weight: 700;
}

tr:nth-child(even) {
    background-color: #f8fafc;
}

.diagram-box {
    background-color: #0f172a;
    color: #38bdf8;
    border-radius: 8px;
    padding: 12px;
    margin: 12px 0;
    font-family: 'Consolas', monospace;
    font-size: 8.5pt;
    line-height: 1.45;
    white-space: pre;
    overflow-x: auto;
    page-break-inside: avoid;
}

.page-break {
    page-break-before: always;
}
</style>
</head>
<body>

<div style="text-align: center; padding-top: 30px; margin-bottom: 20px;">
    <div style="display: inline-block; background: #e0f2fe; color: #0284c7; font-weight: bold; font-size: 9.5pt; padding: 4px 14px; border-radius: 20px; margin-bottom: 10px;">
        DirectX 12 & DXR Path Tracer Engine Architecture Guide
    </div>
    <h1 class="doc-title">CustomEngine 소스 코드 전체 심층 해설서</h1>
    <div class="doc-subtitle">DirectX 12 입문자를 위한 파트별 완벽 구현 가이드 &amp; 코드 워크스루</div>
</div>

<div class="meta-box">
    <div class="meta-item"><strong>언어 / 표준:</strong> C++20 (MSVC /W4), HLSL SM 6.3/6.5</div>
    <div class="meta-item"><strong>그래픽스 API:</strong> DirectX 12 Core, DXR Tier 1.1</div>
    <div class="meta-item"><strong>문서 버전:</strong> v1.0.0 (Report Edition)</div>
</div>

<div class="callout callout-info">
    <strong>📌 본 문서의 목적과 독자 안내</strong>
    이 문서는 CustomEngine의 전체 소스 코드(Win32 창 생성, 카메라 수학, DXC 셰이더 컴파일러, 지오메트리 버퍼, DXR 2단계 가속 구조, SBT 메모리 정렬, HLSL 패스 트레이싱, 프레임 렌더 루프 및 동기화)를 <strong>DirectX 12를 처음 접하는 초심자의 눈높이</strong>에서 한 줄 한 줄 원리와 배경을 명쾌하게 해설합니다. 모든 코드는 IDE(통합개발환경)에서 보는 것과 동일하게 구문 강조(Syntax Highlighting) 및 깨짐 방지 래핑이 적용되어 있습니다.
</div>

<h2 class="part-header"><span class="part-tag">Part 1</span> DirectX 12 핵심 아키텍처와 개념 기초</h2>

<p>
DirectX 11 이전의 그래픽스 API는 드라이버가 배후에서 수많은 일(메모리 관리, 스레드 동기화, 하드웨어 파이프라인 상태 유추)을 대신 처리해 주는 <em>블랙박스(Black Box)</em> 형태였습니다. 반면 <strong>DirectX 12</strong>는 드라이버 오버헤드를 극도로 줄이고 개발자가 GPU의 하드웨어를 직접 제어할 수 있도록 완전히 재설계된 <em>명시적(Explicit) 로우레벨 API</em>입니다.
</p>

<h3 class="section-header">1.1 DirectX 12의 7대 핵심 객체</h3>
<p>DirectX 12로 렌더링을 수행하기 위해서는 아래의 7대 핵심 개념과 객체를 반드시 이해해야 합니다:</p>

<table>
    <thead>
        <tr>
            <th style="width: 25%;">핵심 객체 / 인터페이스</th>
            <th style="width: 35%;">역할 및 설명</th>
            <th style="width: 40%;">초보자를 위한 비유</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td><strong>Device</strong> (<code>ID3D12Device5</code>)</td>
            <td>가상 그래픽 어댑터. 리소스 생성, 파이프라인 생성, DXR 지원 검사 등 모든 객체 할당의 공장(Factory) 역할.</td>
            <td>그래픽 카드를 통제하는 총사령부</td>
        </tr>
        <tr>
            <td><strong>Command Queue / Allocator / List</strong></td>
            <td>명령 리스트(Command List)에 그리기 명령들을 기록하고, 메모리 풀인 할당자(Allocator)를 거쳐, 실제 GPU 큐(Command Queue)로 전송.</td>
            <td>작업 지시서(List)를 결재판(Allocator)에 모아 GPU 작업라인(Queue)에 투입</td>
        </tr>
        <tr>
            <td><strong>Swap Chain</strong> (<code>IDXGISwapChain3</code>)</td>
            <td>화면에 렌더링 결과물을 표시하기 위한 더블/트리플 버퍼 교환 장치. 티어링 방지 및 프레젠테이션.</td>
            <td>앞면 도화지에 그림을 보여주는 동안 뒷면 도화지에 다음 프레임을 그리는 교환대</td>
        </tr>
        <tr>
            <td><strong>Descriptor Heap</strong> (서술자 힙)</td>
            <td>텍스처, 버퍼, 상수 버퍼를 셰이더가 참조할 수 있도록 뷰(View)들의 메타데이터 포인터를 담는 GPU 메모리 배열.</td>
            <td>셰이더가 볼 수 있는 책들의 인덱스 목차 카드함</td>
        </tr>
        <tr>
            <td><strong>Root Signature</strong> (루트 시그니처)</td>
            <td>셰이더 레지스터(b0, t0, u0 등)와 서술자 힙/상수 버퍼의 매핑 관계를 선언하는 계약서.</td>
            <td>함수의 매개변수 선언부 (어떤 인자가 어디로 전달되는지 명시)</td>
        </tr>
        <tr>
            <td><strong>Pipeline State (PSO / State Object)</strong></td>
            <td>셰이더, 블렌딩, 래스터라이저, 깊이/스텐실 상태를 하나로 결합하여 하드웨어에 고정(Immutable)한 상태 블록.</td>
            <td>공장 조립 라인의 완전 세팅된 금형 틀</td>
        </tr>
        <tr>
            <td><strong>Fence &amp; Resource Barrier</strong></td>
            <td>CPU-GPU 동기화를 위한 카운터(Fence)와, 자원의 상태(읽기/쓰기/복사/출력) 전이를 GPU에 알리는 안전장치(Barrier).</td>
            <td>충돌을 막는 신호등과 공정 전환 알림판</td>
        </tr>
    </tbody>
</table>

<h3 class="section-header">1.2 DXR (DirectX Raytracing)과 2단계 가속 구조</h3>
<p>
기존 래스터라이제이션(Rasterization)이 3차원 삼각형들을 화면 2차원 평면에 투영(Projection)하여 픽셀을 채우는 방식이었다면, <strong>DXR(DirectX Raytracing)</strong>은 카메라에서 화면의 각 픽셀을 향해 가상의 광선(Ray)을 쏘고, 광선이 3D 월드의 삼각형과 교차(Intersection)하는지 수학적으로 검사합니다.
</p>

<div class="diagram-box">
[Top-Level Acceleration Structure (TLAS)]  <-- 월드 내 객체들의 인스턴스 (변환 행렬, 마스크)
         |
         +--> Instance 0  ---> [Bottom-Level AS (BLAS 0)] ---> 메쉬 정점/인덱스 (Cube)
         +--> Instance 1  ---> [Bottom-Level AS (BLAS 1)] ---> 메쉬 정점/인덱스 (Sphere)
</div>

<ul>
    <li><strong>BLAS (Bottom-Level Acceleration Structure)</strong>: 개별 3D 메쉬의 정점(Vertex)과 인덱스(Index) 기하 정보를 포함하는 트리 구조. 로컬 공간(Local Space) 기준.</li>
    <li><strong>TLAS (Top-Level Acceleration Structure)</strong>: 월드 공간(World Space) 내에 배치된 BLAS들의 인스턴스(Instance) 목록과 3x4 아핀 변환 행렬 정보를 담는 최상위 트리.</li>
</ul>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 2</span> 애플리케이션 진입점 및 윈도우 생성 (main &amp; Window)</h2>

<p>
프로그램의 시작은 Win32 진입점 <code>wWinMain</code>에서 시작됩니다. 그래픽스 애플리케이션은 윈도우 OS의 이벤트 루프(Message Pump)와 긴밀하게 연동되어야 하며, 시간의 흐름(Delta Time)을 마이크로초 단위로 정밀하게 측정해야 합니다.
</p>

<h3 class="section-header">2.1 진입점 및 게임 루프 분석 (src/main.cpp)</h3>
""")

    template_parts.append(block_main_cpp)

    template_parts.append("""
<h4 class="subsection-header">핵심 라인별 해설:</h4>
<ul>
    <li><strong>라인 9~12 (헤드리스 스모크 테스트 판정)</strong>:
        <code>GetCommandLineW()</code>를 통해 명령줄 인자를 읽고, 만약 <code>--smoke-test</code>가 포함되어 있다면 5프레임만 렌더링하고 자동으로 정상 종료(Exit code 0)하도록 설계되었습니다. 이는 CI/CD 파이프라인이나 자동화 루프에서 GUI 창이 무한정 대기하는 것을 방지하는 핵심 기법입니다.
    </li>
    <li><strong>라인 19~23 (고해상도 타이머)</strong>:
        <code>QueryPerformanceFrequency</code>와 <code>QueryPerformanceCounter</code>는 CPU 하드웨어의 고해상도 타임스탬프 카운터를 읽어옵니다. 표준 <code>timeGetTime</code>이나 <code>std::chrono</code>보다 오차가 적어 프레임 간 <code>deltaTime</code>을 매우 부드럽고 정확하게 계산할 수 있습니다.
    </li>
    <li><strong>라인 27~31 (논블로킹 메시지 처리)</strong>:
        <code>window.ProcessMessages()</code>가 <code>false</code>를 반환(즉, <code>WM_QUIT</code> 수신)하면 렌더링 루프를 즉시 탈출합니다.
    </li>
    <li><strong>라인 38~39 (Update &amp; Render 분리)</strong>:
        카메라 이동, 점프 물리 계산 등의 상태 갱신은 <code>Update(deltaTime)</code>에서, GPU 명령 기록 및 화면 출력은 <code>Render()</code>에서 명확히 분리하여 실행합니다.
    </li>
</ul>

<h3 class="section-header">2.2 Win32 창 클래스 및 윈도우 프로시저 (src/Window.h / .cpp)</h3>
""")

    template_parts.append(block_window_init)

    template_parts.append("""
<div class="callout callout-tip">
    <strong>💡 AdjustWindowRect의 중요성</strong>
    윈도우의 타이틀 바, 외곽선 테두리는 일정한 픽셀 두께를 가집니다. 만약 단순히 <code>CreateWindowExW</code>에 1280x720을 전달하면 실제 그림이 그려지는 클라이언트 영역(Client Area)은 1264x681 등으로 축소되어 렌더 타깃과 종횡비(Aspect Ratio) 왜곡이 발생합니다.
    <code>AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);</code>를 호출하면 원하는 클라이언트 크기가 정확히 나오도록 윈도우 전체 창 크기를 역계산해 줍니다.
</div>
""")

    template_parts.append(block_window_proc)

    template_parts.append("""
<p>
<strong>C++ 인스턴스 바인딩의 비밀:</strong>
정적(static) 함수인 <code>WindowProc</code>은 멤버 변수에 직접 접근할 수 없습니다. 따라서 창이 생성되는 최초 순간(<code>WM_NCCREATE</code>)에 <code>lParam</code>으로 전달된 <code>this</code> 포인터를 <code>SetWindowLongPtr(hwnd, GWLP_USERDATA, ...)</code>를 통해 Win32 윈도우 내부 메모리에 저장합니다. 이후 발생하는 모든 메시지는 <code>GetWindowLongPtr</code>을 통해 <code>Window*</code>를 복원하여 안전하게 처리합니다.
</p>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 3</span> DirectX 12 하드웨어 초기화 심층 분석 (InitDX12)</h2>

<p>
DirectX 12를 구동하기 위한 하드웨어 초기화 과정은 <code>DX12Renderer::InitDX12()</code>에 집약되어 있습니다. 이 함수는 디버그 레이어 켜기부터 그래픽 카드 선택, 명령 큐 생성, 스왑 체인 구축까지를 담당합니다.
</p>

<h3 class="section-header">3.1 디버그 레이어 및 DXR 하드웨어 어댑터 자동 탐색</h3>
""")

    template_parts.append(block_init_adapter)

    template_parts.append("""
<div class="callout callout-warning">
    <strong>⚠️ 디버그 레이어(Debug Layer)는 입문자의 생명줄</strong>
    DirectX 12 API 호출 실패는 기본적으로 아무런 크래시 메시지 없이 침묵하거나 GPU 크래시(TDR)를 일으킵니다. 디버그 모드에서 <code>ID3D12Debug::EnableDebugLayer()</code>를 활성화하면 잘못된 리소스 상태 전이, 잘못된 정렬, 바인딩 누락을 Visual Studio 디버그 출력 창에 상세한 영문 경고로 출력해 줍니다.
</div>

<p>
<strong>DXR 1.1 하드웨어 어댑터 필터링:</strong>
시스템에 인텔/AMD 내장 그래픽과 NVIDIA 외장 그래픽(RTX 시리즈)이 동시에 존재하는 경우가 많습니다. 코드는 <code>EnumAdapters1</code>을 순회하며 소프트웨어 어댑터(WARP)를 건너뛰고, <code>CheckFeatureSupport</code>로 <code>RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED</code>인 하드웨어를 검출하여 <code>m_device</code>로 설정합니다. DXR을 사용하려면 반드시 <code>ID3D12Device5</code> 이상의 인터페이스가 필요합니다.
</p>

<h3 class="section-header">3.2 명령 큐, 스왑 체인 및 RTV 디스크립터 힙</h3>
""")

    template_parts.append(block_init_swapchain)

    template_parts.append("""
<h4 class="subsection-header">스왑 체인과 RTV 생성의 핵심 단계:</h4>
<ol>
    <li><strong><code>D3D12_COMMAND_QUEUE_DESC</code></strong>: 3D 렌더링 및 광선 추적 계산을 수행하는 기본 직접 실행형 큐(<code>D3D12_COMMAND_LIST_TYPE_DIRECT</code>)를 생성합니다.</li>
    <li><strong><code>DXGI_SWAP_CHAIN_DESC1</code></strong>: 화면 깜빡임과 렉을 방지하는 모던 프레젠테이션 모델인 <code>DXGI_SWAP_EFFECT_FLIP_DISCARD</code>를 설정합니다. 버퍼 개수는 2(더블 버퍼링)입니다.</li>
    <li><strong><code>D3D12_DESCRIPTOR_HEAP_DESC</code></strong>: RTV(Render Target View)를 위한 서술자 힙을 생성합니다. RTV 힙은 GPU 셰이더에서 직접 샘플링하는 용도가 아니므로 <code>D3D12_DESCRIPTOR_HEAP_FLAG_NONE</code>을 사용합니다.</li>
    <li><strong>RTV 뷰 생성 루프</strong>: <code>m_swapChain-&gt;GetBuffer(n, ...)</code>로 각 백버퍼의 <code>ID3D12Resource</code> 핸들을 얻고, <code>CreateRenderTargetView</code>를 호출하여 힙의 핸들에 순차적으로 바인딩합니다. 이때 다음 슬롯 주소는 <code>m_rtvDescriptorSize</code>만큼 포인터를 증가시켜 결정합니다.</li>
</ol>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 4</span> 3D 수학, 카메라 및 물리 시뮬레이션 (Camera)</h2>

<p>
패스 트레이싱에서 카메라는 단순히 물체를 보는 관점이 아니라, <strong>광선이 출발하는 원점(Origin)과 방향(Direction)을 결정하는 핵심 수학 엔진</strong>입니다.
</p>

<h3 class="section-header">4.1 DirectXMath의 메모리 정렬과 벡터 연산 규칙</h3>
<p>
<code>DirectXMath</code>는 x86/x64의 SSE/AVX SIMD 레지스터(128비트)를 활용하여 4개의 부동소수점 연산을 한 번의 CPU 사이클에 수행합니다.
</p>
<ul>
    <li><code>XMVECTOR</code> / <code>XMMATRIX</code>: SIMD 레지스터에 상주하는 16바이트 정렬 타입입니다. 구조체의 멤버 변수로 저장하면 메모리 정렬 위반으로 충돌할 수 있으므로, <strong>지역 변수 연산용</strong>으로만 사용합니다.</li>
    <li><code>XMFLOAT3</code> / <code>XMFLOAT4X4</code>: 클래스 멤버 변수 저장용 타입입니다. 연산 시에는 <code>XMLoadFloat3</code>로 불러오고, 결과는 <code>XMStoreFloat3</code>로 다시 저장합니다.</li>
</ul>

<h3 class="section-header">4.2 오일러 각을 이용한 방향 벡터 갱신 (src/Camera.cpp)</h3>
""")

    template_parts.append(block_camera_vec)

    template_parts.append("""
<p>
카메라의 전방(Forward) 벡터는 기본 축 (0, 0, 1)에 회전 행렬을 곱하여 얻습니다. 이후 상단(Up, 0, 1, 0) 벡터와 전방 벡터의 <strong>외적(Cross Product)</strong>을 취해 우측(Right) 벡터를 구하고, 다시 외적을 취해 상호 직교(Orthogonal)하는 정규화 축들을 확립합니다.
</p>

<h3 class="section-header">4.3 1인칭 이동 및 점프 물리 시뮬레이션</h3>
""")

    template_parts.append(block_camera_phys)

    template_parts.append("""
<div class="callout callout-info">
    <strong>📐 XZ 평면 이동 투영 (Plane Projection)</strong>
    카메라가 하늘이나 땅을 보고 있을 때(Pitch 각도가 클 때) 단순히 전방(Forward) 벡터로 이동하면 카메라가 공중으로 날아가거나 땅으로 파고듭니다. <code>Camera::Update()</code>에서는 전방 벡터의 Y 성분을 0으로 강제 투영(<code>XMVectorSet(m_forward.x, 0.0f, m_forward.z, 0.0f)</code>)하여 수평 이동을 보장합니다.
</div>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 5</span> 모던 DXC 런타임 셰이더 컴파일러 (ShaderCompiler)</h2>

<p>
과거 DirectX 9~11 시절에는 <code>d3dcompiler_47.dll</code>의 <code>D3DCompile</code> API를 사용했습니다. 그러나 DXR과 최신 Shader Model 6.0 이상을 사용하기 위해서는 LLVM 오픈소스 기반의 모던 컴파일러인 <strong>DirectX Shader Compiler (DXC)</strong>가 필수적입니다.
</p>

<h3 class="section-header">5.1 DXC 컴파일러 아키텍처 및 구현 (src/ShaderCompiler.cpp)</h3>
""")

    template_parts.append(block_shader_compiler)

    template_parts.append("""
<h4 class="subsection-header">핵심 컴파일 인자 플래그 해설:</h4>
<ul>
    <li><code>-E entryPoint</code>: 진입점 함수 이름 지정. DXR 라이브러리(<code>lib_6_3</code>) 컴파일 시에는 개별 진입점이 없으므로 공백(<code>L""</code>)을 전달합니다.</li>
    <li><code>-T targetProfile</code>: 대상 셰이더 프로파일. 레이트레이싱은 <code>lib_6_3</code> 또는 <code>lib_6_5</code>, 버텍스 셰이더는 <code>vs_6_0</code> 등을 지정합니다.</li>
    <li><code>-Zi</code> &amp; <code>-Qembed_debug</code>: 컴파일된 DXIL 바이너리 내부에 PDB 디버깅 정보를 직접 삽입합니다. PIX나 RenderDoc 같은 GPU 디버거로 프레임을 캡처할 때 셰이더 원본 소스 코드를 라인 단위로 볼 수 있게 해줍니다.</li>
    <li><code>-Od</code>: 컴파일러의 코드 최적화를 꺼서 디버깅 시 변수 값이 보존되도록 합니다.</li>
    <li><code>DXC_OUT_ERRORS</code>: 문법 오류 발생 시 문자열 버퍼를 추출하여 콘솔 및 <code>OutputDebugStringA</code>로 출력한 후 C++ 예외를 발생시킵니다.</li>
</ul>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 6</span> 3D 모델 로딩 및 지오메트리 버퍼 생성 (Geometry Buffers)</h2>

<p>
GPU 렌더링의 기본 단위는 정점(Vertex)과 인덱스(Index)입니다. CPU 메모리에 있는 지오메트리 데이터를 고속의 GPU 전용 VRAM으로 옮기기 위해서는 <strong>기본 버퍼 복사 패턴(Default Buffer Copy Pattern)</strong>을 사용해야 합니다.
</p>

<h3 class="section-header">6.1 고속 VRAM 버퍼 생성 패턴 (CreateDefaultBuffer)</h3>
""")

    template_parts.append(block_default_buffer)

    template_parts.append("""
<div class="diagram-box">
[CPU 시스템 메모리]  --- memcpy --->  [UPLOAD 힙 (중간 버퍼)]
                                              |
                                     CopyBufferRegion (GPU 복사)
                                              v
[DEFAULT 힙 (초고속 VRAM)]  <--- ResourceBarrier (COPY_DEST -> NON_PIXEL_SRV)
</div>

<h4 class="subsection-header">왜 업로드 버퍼를 거쳐서 복사해야 하는가?</h4>
<p>
GPU는 <code>D3D12_HEAP_TYPE_DEFAULT</code>로 할당된 비디오 메모리(VRAM)에 가장 빠르고 효율적으로 접근할 수 있습니다. 하지만 CPU는 이 메모리에 직접 쓸 수 없습니다. 따라서 CPU가 쓸 수 있는 <code>D3D12_HEAP_TYPE_UPLOAD</code> 임시 버퍼에 먼저 복사한 뒤, GPU 명령 리스트의 <code>CopyBufferRegion</code> 명령을 통해 초고속 복사를 수행하고 리소스 장벽을 통해 읽기 가능 상태로 전환합니다.
</p>

<h3 class="section-header">6.2 Wavefront OBJ 파싱 및 트라이앵글 분할 (LoadModel)</h3>
""")

    template_parts.append(block_load_model)

    template_parts.append("""
<p>
3D 모델 파일(<code>assets/cube.obj</code>)의 면(Face, <code>f</code>) 데이터는 사각형(Quad, 정점 4개: 0 1 2 3)으로 구성되어 있습니다. GPU의 DXR 하드웨어는 기본적으로 삼각형만 지원하므로, 인덱스를 <code>0-1-2</code>와 <code>0-2-3</code> 두 개의 삼각형으로 분할(Triangulation)하여 인덱스 버퍼를 구축합니다.
</p>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 7</span> DXR 2단계 가속 구조 (BLAS &amp; TLAS) 완전 정복</h2>

<p>
DXR의 심장인 가속 구조(Acceleration Structure)를 구축하는 과정입니다. 이 단계에서 GPU 내부의 RT Core가 활용할 수 있는 최적화된 공간 분할 트리(BVH)가 생성됩니다.
</p>

<h3 class="section-header">7.1 BLAS 구축 (Bottom-Level Acceleration Structure)</h3>
""")

    template_parts.append(block_blas)

    template_parts.append("""
<p>
정점 버퍼의 시작 GPU 가상 주소(<code>VertexBuffer.StartAddress</code>)와 인덱스 버퍼 주소(<code>IndexBuffer</code>)를 <code>D3D12_RAYTRACING_GEOMETRY_DESC</code>에 기술합니다.
<code>GetRaytracingAccelerationStructurePrebuildInfo</code>를 호출하면 드라이버가 BVH 빌드에 필요한 임시 메모리(<code>ScratchDataSizeInBytes</code>)와 최종 결과물 버퍼 크기(<code>ResultDataMaxSizeInBytes</code>)를 계산해 줍니다.
</p>

<h3 class="section-header">7.2 TLAS 구축 및 인스턴스 기술자 매핑</h3>
""")

    template_parts.append(block_tlas)

    template_parts.append("""
<h4 class="subsection-header">핵심 구조체 분석: D3D12_RAYTRACING_INSTANCE_DESC</h4>
<ul>
    <li><code>Transform[3][4]</code>: 3D 공간 상에서 객체의 위치, 회전, 크기를 나타내는 3x4 아핀 변환 행렬입니다. 여기서는 단위 행렬(Identity)이 적용되었습니다.</li>
    <li><code>InstanceMask = 0xFF</code>: 광선 발사 시 특정 마스크와 비트 AND 연산을 수행하여 어떤 인스턴스를 무시할지 필터링할 수 있습니다. <code>0xFF</code>는 모든 광선과 충돌합니다.</li>
    <li><code>InstanceContributionToHitGroupIndex = 0</code>: 이 인스턴스에 광선이 충돌했을 때 SBT에서 참조할 히트 그룹(Hit Group)의 기본 오프셋입니다.</li>
    <li><code>AccelerationStructure</code>: 대상 메쉬의 BLAS가 위치한 GPU 가상 주소(<code>m_bottomLevelAS-&gt;GetGPUVirtualAddress()</code>)를 지정합니다.</li>
</ul>

<div class="callout callout-danger">
    <strong>🛑 필수 동기화: UAV 리소스 장벽 (UAV Barrier)</strong>
    BLAS 빌드와 TLAS 빌드는 모두 GPU 상에서 비동기로 실행됩니다. 만약 BLAS 빌드가 끝나기 전에 TLAS 빌드가 시작되면 깨진 지오메트리를 참조하여 크래시가 발생합니다. 코드의 <code>D3D12_RESOURCE_BARRIER_TYPE_UAV</code> 장벽은 앞선 빌드가 완전히 끝날 때까지 GPU 하드웨어를 안전하게 대기시킵니다.
</div>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 8</span> DXR 파이프라인(RTPSO) &amp; 루트 시그니처</h2>

<p>
기존 래스터라이제이션이 단일 <code>ID3D12PipelineState</code>를 사용했다면, DXR은 여러 셰이더와 라이브러리가 유기적으로 연결된 거대한 상태 객체인 <strong>State Object (RTPSO)</strong>를 사용합니다.
</p>

<h3 class="section-header">8.1 글로벌 루트 시그니처 (Global Root Signature)</h3>
""")

    template_parts.append(block_root_sig)

    template_parts.append("""
<p>
루트 파라미터는 셰이더에 데이터를 전달하는 통로입니다:
</p>
<ul>
    <li><strong>슬롯 0 (Descriptor Table)</strong>: UAV 2개 (<code>u0</code>: 화면 출력 렌더 타깃, <code>u1</code>: 다중 프레임 색상 누적 버퍼).</li>
    <li><strong>슬롯 1 (Root SRV)</strong>: TLAS 가속 구조 (<code>t0</code>).</li>
    <li><strong>슬롯 2 (Root SRV)</strong>: 메쉬 정점 버퍼 구조화 버퍼 (<code>t1</code>).</li>
    <li><strong>슬롯 3 (Root SRV)</strong>: 메쉬 인덱스 버퍼 구조화 버퍼 (<code>t2</code>).</li>
    <li><strong>슬롯 4 (Root CBV)</strong>: 카메라 역행렬 및 프레임 번호를 담은 상수 버퍼 (<code>b0</code>).</li>
</ul>

<h3 class="section-header">8.2 DXR State Object 5대 하위 객체 (CreateRaytracingPipeline)</h3>
""")

    template_parts.append(block_rtpso)

    template_parts.append("""
<div class="callout callout-warning">
    <strong>⚠️ DXR 입문자가 가장 많이 저지르는 버그 1위: 페이로드 바이트 크기 불일치</strong>
    <code>shaderConfig.MaxPayloadSizeInBytes = 10 * sizeof(float);</code>는 정확히 40바이트입니다.
    HLSL의 <code>struct RayPayload</code> (<code>float3 color(12) + float hitT(4) + float3 normal(12) + float3 albedo(12) = 40 bytes</code>)와 단 1바이트라도 다르면 런타임에 <code>DXGI_ERROR_DEVICE_REMOVED</code> 크래시가 발생합니다!
</div>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 9</span> 셰이더 바인딩 테이블 (SBT: Shader Binding Table)</h2>

<p>
광선이 허공을 날아갈 때(Miss), 물체에 부딪혔을 때(Closest Hit), 혹은 최초로 발사될 때(RayGen) 어떤 셰이더 코드를 실행해야 할까요? GPU는 CPU처럼 함수 포인터를 직접 호출할 수 없으므로, 메모리에 정렬된 테이블을 만들어 둡니다. 이것이 바로 <strong>셰이더 바인딩 테이블(SBT)</strong>입니다.
</p>

<h3 class="section-header">9.1 메모리 정렬 및 SBT 레이아웃 (src/DX12Renderer.cpp)</h3>
""")

    template_parts.append(block_sbt)

    template_parts.append("""
<div class="diagram-box">
[Shader Binding Table (SBT) 메모리 레이아웃 (단일 UPLOAD 버퍼)]
+------------------------+------------------------+------------------------+
| Entry 0: RayGen Record | Entry 1: Miss Record   | Entry 2: HitGroup Rec  |
| (Offset: 0B)           | (Offset: 64B)          | (Offset: 128B)         |
+------------------------+------------------------+------------------------+
| 32B Shader Identifier  | 32B Shader Identifier  | 32B Shader Identifier  |
| 32B Padding            | 32B Padding            | 32B Padding            |
+------------------------+------------------------+------------------------+
</div>

<h4 class="subsection-header">하드웨어 정렬 공식:</h4>
<p>
DirectX 12 스펙상 각 셰이더 레코드는 최소 32바이트(<code>D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT</code>) 단위여야 하며, 개별 셰이더 테이블의 시작 주소는 64바이트(<code>D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT</code>) 정렬이어야 합니다. 코드의 다음 비트 연산이 이를 완벽히 보장합니다:
</p>
<pre><code>m_sbtEntrySize = (m_sbtEntrySize + (64 - 1)) &amp; ~(64 - 1); // 32 -> 64바이트로 올림 정렬</code></pre>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 10</span> DXR 패스 트레이싱 HLSL 셰이더 완전 정복</h2>

<p>
이제 모든 수학과 알고리즘이 실행되는 셰이더 코드(<code>shaders/raytracing.hlsl</code>)를 분석합니다. 이 셰이더는 단순한 단일 광선 검출을 넘어, 물리 기반의 다중 바운스 몬테카를로 경로 추적(Monte Carlo Path Tracing)과 안티에일리어싱, 시간적 누적 버퍼, 톤매핑을 모두 수행합니다.
</p>

<h3 class="section-header">10.1 PCG 난수 생성기 및 반구 코사인 샘플링</h3>
""")

    template_parts.append(block_hlsl_rng)

    template_parts.append("""
<p>
<strong>반구 코사인 샘플링(Cosine-weighted Hemisphere Sampling):</strong><br>
빛이 거친 표면(디퓨즈/난반사)에 부딪히면 램버트 코사인 법칙(cos &theta;)에 비례하여 모든 방향으로 산란됩니다. 균일(Uniform)하게 무작위 방향을 뽑으면 샘플링 효율이 떨어지므로, 표면 법선(N) 축을 중심으로 코사인 가중치가 적용된 X, Y, Z 방향 벡터를 구면 좌표계로부터 유도합니다.
</p>

<h3 class="section-header">10.2 RayGen 셰이더: 4회 바운스 패스 트레이싱 &amp; 시간적 누적</h3>
""")

    template_parts.append(block_hlsl_raygen)

    template_parts.append("""
<h4 class="subsection-header">RayGen의 5단계 파이프라인:</h4>
<ol>
    <li><strong>서브픽셀 지터링 (Anti-Aliasing)</strong>: 픽셀 중심 대신 픽셀 영역 내부의 무작위 위치(<code>jitter</code>)를 조준하여 계단 현상(Aliasing)을 제거합니다.</li>
    <li><strong>역행렬을 통한 1차 광선 생성</strong>: 화면 정규화 좌표(NDC)에 <code>projInverse</code>와 <code>viewInverse</code>를 곱하여 월드 공간상의 광선 원점과 방향을 복원합니다.</li>
    <li><strong>4회 바운스 루프</strong>: <code>TraceRay</code>를 호출하여 충돌 지점을 찾고, 충돌 시 해당 지점의 법선 기준으로 새로운 반사 광선을 발사합니다. 광선이 반사될 때마다 물체의 표면 색상(<code>payload.albedo</code>)이 스루풋(<code>throughput</code>)에 누적 곱해집니다.</li>
    <li><strong>시간적 누적 (Temporal Accumulation)</strong>:
        패스 트레이싱은 노이즈가 많습니다. 카메라가 정지해 있을 때 매 프레임의 광선 결과를 평균 누적합니다:
        <code>C_new = lerp(C_prev, C_curr, 1.0f / float(frameCount))</code>
    </li>
    <li><strong>라인하르트 톤매핑 &amp; 감마 보정</strong>:
        물리적으로 계산된 HDR 색상을 LDR 모니터에 표현하기 위해 <code>C / (C + 1.0)</code> 연산을 수행하고, 모니터의 비선형 밝기 특성에 맞추어 감마(1.0 / 2.2) 보정을 적용합니다.
    </li>
</ol>

<h3 class="section-header">10.3 Miss &amp; ClosestHit 셰이더</h3>
""")

    template_parts.append(block_hlsl_hit)

    template_parts.append("""
<p>
<code>ClosestHit</code>에서는 DXR 내장 인트린식 함수인 <code>PrimitiveIndex()</code>를 통해 삼각형 인덱스를 읽고, 정점 버퍼에서 3개의 정점을 가져와 <strong>벡터 외적(Cross Product)</strong>으로 정확한 표면 법선(Normal)을 실시간으로 계산해 냅니다.
</p>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 11</span> 프레임 렌더 루프 및 GPU 동기화 (Update &amp; Render)</h2>

<p>
매 프레임마다 실행되는 <code>DX12Renderer::Update</code>와 <code>Render</code>는 CPU와 GPU의 완벽한 오케스트레이션을 보여줍니다.
</p>

<h3 class="section-header">11.1 Update 및 상수 버퍼 갱신</h3>
""")

    template_parts.append(block_render_update)

    template_parts.append("""
<p>
카메라가 조금이라도 움직이면 기존에 누적된 광선 데이터는 잔상(Ghosting)을 일으키므로 <code>m_accumulationFrames = 0;</code>으로 리셋합니다. 카메라의 뷰/투영 행렬의 역행렬을 구하고, HLSL 셰이더의 열 우선(Column-major) 행렬 곱셈 규격에 맞추어 <code>XMMatrixTranspose</code>로 전치하여 매핑된 메모리에 직접 씁니다.
</p>

<h3 class="section-header">11.2 Render 및 다단계 자원 장벽 (Resource Barrier)</h3>
""")

    template_parts.append(block_render_render)

    template_parts.append("""
<div class="diagram-box">
[프레임 렌더링 자원 전이 및 흐름도]
1. m_raytracingOutput: [COPY_SOURCE] ---- Transition Barrier ----> [UNORDERED_ACCESS (UAV)]
2. DispatchRays 실행 (광선 연산 수행)
3. m_raytracingOutput: [UNORDERED_ACCESS] ---- Transition Barrier ----> [COPY_SOURCE]
   스왑체인 백버퍼:   [PRESENT]          ---- Transition Barrier ----> [COPY_DEST]
4. CopyResource (UAV 출력물 -> 백버퍼 고속 복사)
5. 스왑체인 백버퍼:   [COPY_DEST]        ---- Transition Barrier ----> [PRESENT]
6. Present(1, 0) 호출 (화면 주사율에 맞춰 송출)
</div>

<h3 class="section-header">11.3 펜스(Fence)를 통한 GPU 플러시 (FlushCommandQueue)</h3>
""")

    template_parts.append(block_render_flush)

    template_parts.append("""
<p>
<code>m_commandQueue-&gt;Signal(m_fence.Get(), fenceToWaitFor)</code>는 GPU 명령 큐의 가장 마지막 작업 뒤에 펜스 카운터를 증가시키는 마커를 삽입합니다. CPU는 <code>WaitForSingleObject</code>로 GPU가 해당 지점에 도달할 때까지 안전하게 대기하여 레이스 컨디션과 메모리 누수를 방지합니다.
</p>

<div class="page-break"></div>
<h2 class="part-header"><span class="part-tag">Part 12</span> 아키텍처 다이어그램 및 DX12 트러블슈팅 가이드</h2>

<h3 class="section-header">12.1 전체 시스템 메모리 맵 및 데이터 파이프라인</h3>
<div class="diagram-box">
+-----------------------------------------------------------------------------------------+
|                                CPU 시스템 메모리                                        |
|  [main.cpp] -> [Camera (위치, 회전)] -> ConstantBuffer -> [m_cbvDataBegin (Map)]       |
|  [Window.cpp] -> Win32 Event Pump -> Update(dt) -> Render()                            |
+-----------------------------------------------------------------------------------------+
                                          | (PCIe Bus Transfer)
                                          v
+-----------------------------------------------------------------------------------------+
|                              GPU 전용 메모리 (VRAM)                                     |
|  +-----------------------+  +------------------------+  +----------------------------+  |
|  | BLAS (Cube Mesh Vert) |  | TLAS (World Instance)  |  | SBT (RayGen/Miss/HitGroup) |  |
|  +-----------------------+  +------------------------+  +----------------------------+  |
|             \                            /                           /                  |
|              v                          v                           v                   |
|  +-----------------------------------------------------------------------------------+  |
|  | DXR RTPSO & DispatchRays -> HLSL SM 6.3 (RayGen / ClosestHit / Miss)              |  |
|  +-----------------------------------------------------------------------------------+  |
|             |                                            |                              |
|             v (Write)                                    v (Read / Lerp)                |
|  [u0: RaytracingOutput] <---------------------- [u1: AccumulationBuffer]                 |
|             | (CopyResource)                                                            |
|             v                                                                           |
|  [SwapChain BackBuffer] ----> Present(1, 0) ----> [디스플레이 모니터]                   |
+-----------------------------------------------------------------------------------------+
</div>

<h3 class="section-header">12.2 DirectX 12 개발자가 자주 겪는 실수 TOP 5 &amp; 해결책</h3>
<table>
    <thead>
        <tr>
            <th style="width: 25%;">오류 현상</th>
            <th style="width: 35%;">근본 원인</th>
            <th style="width: 40%;">검증 및 해결 방법</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td><strong>DEVICE_REMOVED (TDR 크래시)</strong></td>
            <td>HLSL 무한 루프, 잘못된 메모리 주소 역참조, 또는 <code>RayPayload</code> 크기 불일치.</td>
            <td>디버그 레이어 메시지 확인 및 <code>MaxPayloadSizeInBytes</code>와 HLSL 구조체 바이트 크기 일치 검증.</td>
        </tr>
        <tr>
            <td><strong>화면이 검은색으로만 나옴</strong></td>
            <td>리소스 장벽 누락으로 백버퍼에 복사되지 않았거나, 카메라 역행렬의 전치(Transpose) 누락.</td>
            <td>UAV -> <code>COPY_SOURCE</code> 및 <code>COPY_DEST</code> -> <code>PRESENT</code> 전이 확인, <code>XMMatrixTranspose</code> 호출 확인.</td>
        </tr>
        <tr>
            <td><strong>상수 버퍼 바인딩 크래시</strong></td>
            <td>상수 버퍼 크기가 256바이트 배수로 정렬되지 않음.</td>
            <td><code>(sizeof(T) + 255) &amp; ~255</code> 매크로를 사용하여 256바이트 패딩 보장.</td>
        </tr>
        <tr>
            <td><strong>SBT 바인딩 에러</strong></td>
            <td>레코드 크기가 32바이트 또는 테이블 시작 주소가 64바이트로 정렬되지 않음.</td>
            <td><code>D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT</code>(64)로 올림 정렬.</td>
        </tr>
        <tr>
            <td><strong>간헐적 화면 떨림 / 티어링</strong></td>
            <td>스왑 체인 플립 전후 CPU와 GPU 간의 동기화 부재.</td>
            <td><code>FlushCommandQueue()</code> 또는 프레임별 펜스를 사용하여 명령 완료 대기.</td>
        </tr>
    </tbody>
</table>

<div style="margin-top: 40px; text-align: center; color: #64748b; font-size: 9pt; border-top: 1px solid #e2e8f0; padding-top: 16px;">
    CustomEngine DirectX 12 &amp; DXR Path Tracer Architecture Deep-Dive Guide • Generated Automatically
</div>

</body>
</html>
""")

    full_html = "".join(template_parts)
    print(f"Writing complete HTML guide to: {HTML_OUTPUT}")
    with open(HTML_OUTPUT, "w", encoding="utf-8") as f:
        f.write(full_html)

    print(f"HTML size: {HTML_OUTPUT.stat().st_size} bytes")

    print("Compiling HTML to PDF using Microsoft Edge Headless...")

    cmd = [
        EDGE_PATH,
        "--headless=new",
        "--disable-gpu",
        "--run-all-compositor-stages-before-draw",
        "--no-pdf-header-footer",
        f"--print-to-pdf={PDF_OUTPUT}",
        str(HTML_OUTPUT.resolve())
    ]

    proc = subprocess.run(cmd, capture_output=True, text=True)

    if PDF_OUTPUT.exists():
        pdf_size_kb = PDF_OUTPUT.stat().st_size / 1024
        print(f"[SUCCESS] PDF successfully created: {PDF_OUTPUT} ({pdf_size_kb:.2f} KB)")
    else:
        print(f"[ERROR] PDF generation failed!")
        print(f"STDOUT: {proc.stdout}")
        print(f"STDERR: {proc.stderr}")
        sys.exit(1)

if __name__ == "__main__":
    build_report()
