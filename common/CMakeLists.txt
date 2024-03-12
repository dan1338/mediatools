cmake_minimum_required(VERSION 3.14)

project(common)

add_library(common)
target_include_directories(common
    PUBLIC
    .)
target_sources(common
    PRIVATE
    ./JpegLs.cpp
    ./transport/IpVideoServer.cpp)

