cmake_minimum_required(VERSION 3.12)

create_exe(
	NAME "gpmake"
	SOURCES
		"main.cpp"
	LIBS
		project::dependencies
	)

create_exe(NAME gpmake2
	SOURCES "main2.cpp"
	)
