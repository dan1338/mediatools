cmake_minimum_required(VERSION 3.14)

project(common)

add_library(common INTERFACE)
target_include_directories(common
	PUBLIC
	INTERFACE
	.)

