cmake_minimum_required(VERSION 3.14)

project(stream_video)

add_executable(stream_video)
target_link_libraries(stream_video PRIVATE LibUVC::UVC)
target_sources(stream_video
    PRIVATE
    ./main.cpp
    ./video_source/UVCVideoSource.cpp
    ./video_source/IVideoSource.cpp)

