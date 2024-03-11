cmake_minimum_required(VERSION 3.14)

project(stream_video)

add_executable(stream_video)
target_link_libraries(stream_video 
	PRIVATE 
	LibUVC::UVC
	common)
target_sources(stream_video
    PRIVATE
	./VideoSource/UVCVideoSource.cpp
	./VideoSource/IVideoSource.cpp
	./main.cpp)
