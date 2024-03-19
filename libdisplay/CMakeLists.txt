cmake_minimum_required(VERSION 3.14)

project(display)

find_package(glfw3 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(gles2 REQUIRED glesv2)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#add_library(display)
add_executable(display)
target_include_directories(display
    PUBLIC
    .)
target_link_libraries(display
    PUBLIC
    glfw
    ${gles2_LIBRARIES}
    common)
target_sources(display
    PRIVATE
    ./main.cpp
    ./OpenGLVideoDisplay.cpp)
