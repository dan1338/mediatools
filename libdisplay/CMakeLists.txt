cmake_minimum_required(VERSION 3.14)

project(display)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(display)
target_include_directories(display
    PUBLIC
    .)

if (DEFINED ANDROID_ABI)
    target_link_libraries(display
            PUBLIC
            GLESv2
            ${gles2_LIBRARIES}
            common)
else()
    find_package(glfw3 REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(gles2 REQUIRED glesv2)
    target_link_libraries(display
            PUBLIC
            glfw
            ${gles2_LIBRARIES}
            common)
endif()

target_sources(display
    PRIVATE
	#./main.cpp
    ./OpenGLVideoDisplay.cpp)
