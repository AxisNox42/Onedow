# ─────────────────────────────────────────────────────────────
# WiNILL — 크로스플랫폼 빌드 (macOS / Linux)
#   Windows 는 Visual Studio (WiNILL/WiNILL.vcxproj) 로 빌드.
#
# macOS 사전 준비:
#   brew install cmake glfw
#
# 빌드:
#   cmake -B build -DCMAKE_BUILD_TYPE=Release
#   cmake --build build
#   cd build && ./WiNILL      # (Resource/ 가 실행 파일 옆에 복사됨)
# ─────────────────────────────────────────────────────────────
cmake_minimum_required(VERSION 3.16)
project(WiNILL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(WiNILL
    WiNILL/main.cpp
    WiNILL/GameManager.cpp
    WiNILL/FakeWindow.cpp
    WiNILL/WindowFx.cpp
    WiNILL/stb_impl.cpp
    glad.c
)

# 헤더 경로 — 번들 라이브러리(glad/GLFW/glm/stb) + 게임 소스
target_include_directories(WiNILL PRIVATE
    ${CMAKE_SOURCE_DIR}/Include
    ${CMAKE_SOURCE_DIR}/WiNILL
)

# ── GLFW ──  (brew install glfw → find_package 로 탐지)
find_package(glfw3 3.3 QUIET)
if(glfw3_FOUND)
    target_link_libraries(WiNILL PRIVATE glfw)
else()
    # pkg-config 폴백
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    target_include_directories(WiNILL PRIVATE ${GLFW_INCLUDE_DIRS})
    target_link_libraries(WiNILL PRIVATE ${GLFW_LIBRARIES})
endif()

# ── OpenGL ──
find_package(OpenGL REQUIRED)
target_link_libraries(WiNILL PRIVATE OpenGL::GL)

if(APPLE)
    target_link_libraries(WiNILL PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo")
    # macOS 의 OpenGL deprecation 경고 억제
    target_compile_definitions(WiNILL PRIVATE GL_SILENCE_DEPRECATION)
endif()

# ── 리소스(폰트) 를 실행 파일 옆으로 복사 ──
#   런타임에 "Resource/Font/..." 상대 경로로 로드하므로 작업 디렉터리에 있어야 함
add_custom_command(TARGET WiNILL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/WiNILL/Resource
        $<TARGET_FILE_DIR:WiNILL>/Resource
    COMMENT "Copying Resource/ next to the executable")
