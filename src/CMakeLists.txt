cmake_minimum_required(VERSION 3.12)

create_exe(
	NAME "gpmake"
	SOURCES
		"main.cpp"
	LIBS
		project::dependencies
	)

create_header_lib(
	NAME gpmake_cpp_lib
	DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
	SOURCES "core/queue.hpp"
)



create_exe(
	NAME gpmake2
	SOURCES "main2.cpp"
	LIBS gpmake_cpp_lib
		project::threading
	)
