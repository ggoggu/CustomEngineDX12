cmake_minimum_required(VERSION 3.20)
project(CustomEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 출력 디렉토리 설정
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 소스 파일 추가
file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.h")

add_executable(CustomEngine WIN32 ${SRC_FILES})

# DX12 및 Windows 라이브러리 링크
target_link_libraries(CustomEngine PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
    dxcompiler.lib
    user32.lib
)

# -----------------------------------------------------------------
# 테스트 슈트 (EngineTests)
# -----------------------------------------------------------------
enable_testing()

set(TEST_SOURCES
    tests/TestMain.cpp
    src/Camera.cpp
    src/Camera.h
    src/ShaderCompiler.cpp
    src/ShaderCompiler.h
)

add_executable(EngineTests ${TEST_SOURCES})

target_link_libraries(EngineTests PRIVATE
    d3d12.lib
    dxgi.lib
    dxguid.lib
    d3dcompiler.lib
    dxcompiler.lib
    user32.lib
)

add_test(NAME EngineTests COMMAND EngineTests)

# 컴파일러 경고 수준 등 설정 (MSVC 기준)
if(MSVC)
    target_compile_options(CustomEngine PRIVATE /W4 /MP /Zc:__cplusplus)
    target_compile_definitions(CustomEngine PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

    target_compile_options(EngineTests PRIVATE /W4 /MP /Zc:__cplusplus)
    target_compile_definitions(EngineTests PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
endif()

# 셰이더 폴더를 실행 파일 위치로 복사 (CustomEngine)
add_custom_command(TARGET CustomEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:CustomEngine>/shaders"
)

# 애셋 폴더를 실행 파일 위치로 복사 (CustomEngine)
add_custom_command(TARGET CustomEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/assets"
    "$<TARGET_FILE_DIR:CustomEngine>/assets"
)

# 셰이더 폴더를 테스트 실행 파일 위치로 복사 (EngineTests)
add_custom_command(TARGET EngineTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/shaders"
    "$<TARGET_FILE_DIR:EngineTests>/shaders"
)

# Windows SDK의 dxcompiler.dll 및 dxil.dll 복사 (런타임 DXC 컴파일 종속성 자동 해결)
if(WIN32)
    file(GLOB WINDOWS_SDK_DXC_DLLS "C:/Program Files (x86)/Windows Kits/10/bin/*/x64/dxcompiler.dll")
    if(WINDOWS_SDK_DXC_DLLS)
        list(GET WINDOWS_SDK_DXC_DLLS -1 LATEST_DXCOMPILER_DLL)
        get_filename_component(DXC_BIN_DIR "${LATEST_DXCOMPILER_DLL}" DIRECTORY)
        
        add_custom_command(TARGET CustomEngine POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            "${DXC_BIN_DIR}/dxil.dll"
            "$<TARGET_FILE_DIR:CustomEngine>"
        )
        add_custom_command(TARGET EngineTests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DXC_BIN_DIR}/dxcompiler.dll"
            "${DXC_BIN_DIR}/dxil.dll"
            "$<TARGET_FILE_DIR:EngineTests>"
        )
    endif()
endif()
