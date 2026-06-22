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
    ${WINILL_DIR}/Core/GameManager.cpp
    ${WINILL_DIR}/Core/Scenes/SceneBoot.cpp
    ${WINILL_DIR}/Core/Scenes/SceneSkills.cpp
    ${WINILL_DIR}/Core/Scenes/SceneUI.cpp
    ${WINILL_DIR}/Core/Scenes/Scenes.cpp
    ${WINILL_DIR}/Core/Scenes/WindowChrome.cpp
    ${WINILL_DIR}/Render/Camera.cpp
    ${WINILL_DIR}/Render/EntityDraw.cpp
    ${WINILL_DIR}/Render/MainShader.cpp
    ${WINILL_DIR}/Render/stb_impl.cpp
    ${WINILL_DIR}/System/Audio.cpp
    ${WINILL_DIR}/System/FakeWindow.cpp
    ${WINILL_DIR}/System/Input.cpp
    ${WINILL_DIR}/System/WindowFx.cpp
)

target_include_directories(WiNILL PRIVATE
    ${CMAKE_SOURCE_DIR}/Include
    ${WINILL_DIR}/Core
    ${WINILL_DIR}/Core/Scenes
    ${WINILL_DIR}/Render
    ${WINILL_DIR}/Entity
    ${WINILL_DIR}/Entity/Boss
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
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${WINILL_DIR}/Resource
        $<TARGET_FILE_DIR:WiNILL>/Resource
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${WINILL_DIR}/Icons
        $<TARGET_FILE_DIR:WiNILL>/Icons
    COMMENT "Copying Resource/ and Icons/ next to executable")

# Windows 프로젝트의 Font/ 폴더가 있으면 Resource/Font 로 합침
if(EXISTS ${WINILL_DIR}/Font)
    add_custom_command(TARGET WiNILL POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            $<TARGET_FILE_DIR:WiNILL>/Resource/Font
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${WINILL_DIR}/Font
            $<TARGET_FILE_DIR:WiNILL>/Resource/Font
        COMMENT "Copying Font/ into Resource/Font/")
endif()
