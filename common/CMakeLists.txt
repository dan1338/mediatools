cmake_minimum_required(VERSION 3.14)

project(common)

add_library(common)
target_include_directories(common
    PUBLIC
    .)
target_link_libraries(common
    PUBLIC
    charls)
target_sources(common
    PRIVATE
    ./compression/JpegLs.cpp
    ./transport/IpVideoClient.cpp
    ./transport/IpVideoServer.cpp)

