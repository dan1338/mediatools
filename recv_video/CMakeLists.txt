cmake_minimum_required(VERSION 3.14)

project(recv_video)

add_executable(recv_video)
target_link_libraries(recv_video 
	PRIVATE 
	common)
target_sources(recv_video
    PRIVATE
	./main.cpp)
