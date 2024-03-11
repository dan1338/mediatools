cmake_minimum_required(VERSION 3.14)

project(mediatools)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(libuvc)
add_subdirectory(common)
add_subdirectory(stream_video)

