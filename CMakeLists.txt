# WiNILL / Onedow — macOS · Linux 빌드
# Windows 는 WiNILL/WiNILL.vcxproj (Visual Studio) 사용.
#
# macOS:
#   brew install cmake glfw
#   ./scripts/build_macos.sh
#   ./build/WiNILL
cmake_minimum_required(VERSION 3.16)
project(WiNILL LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(WINILL_DIR ${CMAKE_SOURCE_DIR}/WiNILL)

add_executable(WiNILL
    ${CMAKE_SOURCE_DIR}/glad.c
    ${WINILL_DIR}/main.cpp
    ${WINILL_DIR}/Entity/PlayerRuntime.cpp
    ${WINILL_DIR}/Entity/EnemySpawner.cpp
    ${WINILL_DIR}/Entity/ProjectileSystem.cpp
    ${WINILL_DIR}/Core/GameManager.cpp
    ${WINILL_DIR}/Core/Scenes/SceneBoot.cpp
    ${WINILL_DIR}/Core/Scenes/SceneSkills.cpp
    ${WINILL_DIR}/Core/Scenes/SceneUI.cpp
    ${WINILL_DIR}/Core/Scenes/SceneSettings.cpp
    ${WINILL_DIR}/Core/Scenes/SceneRunConfig.cpp
    ${WINILL_DIR}/Core/Scenes/SceneStart.cpp
    ${WINILL_DIR}/Core/Scenes/ScenePause.cpp
    ${WINILL_DIR}/Core/Scenes/SceneRunEnd.cpp
    ${WINILL_DIR}/Core/Scenes/SceneAugment.cpp
    ${WINILL_DIR}/Core/Scenes/SceneMenus.cpp
    ${WINILL_DIR}/Core/Scenes/WindowChrome.cpp
    ${WINILL_DIR}/Render/Camera.cpp
    ${WINILL_DIR}/Render/BlurShader.cpp
    ${WINILL_DIR}/Render/EntityDraw.cpp
    ${WINILL_DIR}/Render/PlayerWeaponShell.cpp
    ${WINILL_DIR}/Render/GameFonts.cpp
    ${WINILL_DIR}/Render/MainShader.cpp
    ${WINILL_DIR}/Render/NebulaGlowShader.cpp
    ${WINILL_DIR}/Render/PlanetShader.cpp
    ${WINILL_DIR}/Render/stb_impl.cpp
    ${WINILL_DIR}/System/Audio.cpp
    ${WINILL_DIR}/System/DebugToolkit.cpp
    ${WINILL_DIR}/System/FakeWindow.cpp
    ${WINILL_DIR}/System/Input.cpp
    ${WINILL_DIR}/System/WindowFx.cpp
)

if(WIN32)
    set_target_properties(WiNILL PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build/windows/x64/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/build/windows/x64/Release")
endif()

target_include_directories(WiNILL PRIVATE
    ${CMAKE_SOURCE_DIR}/Include
    ${WINILL_DIR}/Core
    ${WINILL_DIR}/Core/Scenes
    ${WINILL_DIR}/Render
    ${WINILL_DIR}/Entity
    ${WINILL_DIR}/System
)

find_package(glfw3 3.3 QUIET)
if(glfw3_FOUND)
    target_link_libraries(WiNILL PRIVATE glfw)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLFW REQUIRED glfw3)
    target_include_directories(WiNILL PRIVATE ${GLFW_INCLUDE_DIRS})
    target_link_libraries(WiNILL PRIVATE ${GLFW_LIBRARIES})
endif()

find_package(OpenGL REQUIRED)
target_link_libraries(WiNILL PRIVATE OpenGL::GL)

if(APPLE)
    target_link_libraries(WiNILL PRIVATE
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreFoundation")
    target_compile_definitions(WiNILL PRIVATE GL_SILENCE_DEPRECATION)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(WiNILL PRIVATE pthread dl m)
endif()

# 실행 파일 옆 리소스 (PlatformChdirToExeDir + 상대경로)
add_custom_command(TARGET WiNILL POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory
        $<TARGET_FILE_DIR:WiNILL>/Resource
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${WINILL_DIR}/Resource
        $<TARGET_FILE_DIR:WiNILL>/Resource
    COMMENT "Copying Resource/ next to executable")
