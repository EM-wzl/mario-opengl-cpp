cmake_minimum_required(VERSION 3.20)
project(ProceduralMario VERSION 1.0 LANGUAGES CXX)

# 是否在配置阶段联网拉取 GLFW/GLM。OpenGL 函数加载由本地 glcore 完成，无需 GLEW。
option(MARIO_FETCH_DEPS "Download GLFW/GLM at configure time" OFF)

# 若未显式指定联网拉取，且工程自带 third_party 依赖，则默认使用它们，实现零联网构建。
if(NOT MARIO_FETCH_DEPS AND NOT MARIO_LOCAL_GLFW_DIR
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/CMakeLists.txt"
   AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm/glm/glm.hpp")
    set(MARIO_LOCAL_GLFW_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw")
    set(MARIO_LOCAL_GLM_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm")
    set(MARIO_USING_VENDORED ON)
    message(STATUS "使用工程内置 third_party 依赖（离线构建，无需联网）")
endif()

# Windows 下 CMake/Ninja 的依赖记录会把源码真实路径写回磁盘；若工程路径含中文，
# FindFirstFileExA 扫描依赖目录时会失败。这里把内置依赖统一复制到英文临时目录，
# 使所有依赖头文件路径保持纯 ASCII，从根上规避该问题。
if(MARIO_USING_VENDORED AND WIN32)
    if(DEFINED ENV{TEMP})
        set(MARIO_ASCII_DEPS "$ENV{TEMP}/procedural_mario_deps")
    else()
        set(MARIO_ASCII_DEPS "${CMAKE_BINARY_DIR}/vendor")
    endif()
    file(MAKE_DIRECTORY "${MARIO_ASCII_DEPS}")
    foreach(dep glfw glm)
        if(NOT EXISTS "${MARIO_ASCII_DEPS}/${dep}/CMakeLists.txt"
           AND NOT EXISTS "${MARIO_ASCII_DEPS}/${dep}/glm/glm.hpp")
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/third_party/${dep}"
                 DESTINATION "${MARIO_ASCII_DEPS}")
        endif()
    endforeach()
    set(MARIO_LOCAL_GLFW_DIR "${MARIO_ASCII_DEPS}/glfw")
    set(MARIO_LOCAL_GLM_DIR "${MARIO_ASCII_DEPS}/glm")
    message(STATUS "Windows 依赖已复制到 ASCII 目录 ${MARIO_ASCII_DEPS}")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MARIO_FETCH_DEPS)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
        GIT_SHALLOW TRUE)
    FetchContent_Declare(glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(glfw glm)
    set(MARIO_GLFW_TARGET glfw)
    set(MARIO_GLM_TARGET glm::glm)
elseif(MARIO_LOCAL_GLFW_DIR AND MARIO_LOCAL_GLM_DIR)
    # 离线构建：直接使用本地已存在的 GLFW 源码与 GLM 头文件目录。
    # MARIO_LOCAL_GLFW_DIR 指向 glfw 仓库根（含 CMakeLists.txt 与 src/），
    # MARIO_LOCAL_GLM_DIR 指向包含 glm/glm.hpp 的目录。
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(${MARIO_LOCAL_GLFW_DIR} ${CMAKE_BINARY_DIR}/glfw_local EXCLUDE_FROM_ALL)
    set(MARIO_GLFW_TARGET glfw)
    add_library(mario_glm INTERFACE)
    target_include_directories(mario_glm INTERFACE ${MARIO_LOCAL_GLM_DIR})
    set(MARIO_GLM_TARGET mario_glm)
else()
    find_package(glfw3 3.3 REQUIRED)
    find_package(glm CONFIG QUIET)
    set(MARIO_GLFW_TARGET glfw)
    if(TARGET glm::glm)
        set(MARIO_GLM_TARGET glm::glm)
    endif()
endif()

find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)

add_executable(mario
    src/main.cpp
    src/game.cpp
    src/input.cpp
    src/renderer.cpp
    src/shader.cpp
    src/texture.cpp
    src/cnfont.cpp
    src/glcore.cpp
    src/level.cpp
    src/physics.cpp
    src/player.cpp
    src/enemy.cpp
    src/item.cpp
    src/platform.cpp
    src/progression.cpp
    src/audio.cpp
)

target_include_directories(mario PRIVATE src)
target_link_libraries(mario PRIVATE
    ${MARIO_GLFW_TARGET}
    ${MARIO_GLM_TARGET}
    OpenGL::GL
    Threads::Threads
)

if(WIN32)
    target_link_libraries(mario PRIVATE winmm gdi32)
else()
    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        target_link_libraries(mario PRIVATE ALSA::ALSA)
        target_compile_definitions(mario PRIVATE MARIO_HAS_ALSA=1)
    endif()
endif()

if(MSVC)
    target_compile_options(mario PRIVATE /W4 /permissive-)
else()
    target_compile_options(mario PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 让从源码目录启动程序时也能清楚地区分构建产物。
set_target_properties(mario PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
