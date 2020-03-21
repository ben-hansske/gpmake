cmake_minimum_required(VERSION 3.12)
project(gpmake LANGUAGES CXX VERSION 0.1)

option(USE_CONAN "Use Conan C/C++ package manager instead of find_package" OFF)

include(cmake/config.cmake)
setup_config(cxx_std_17)

include(cmake/sanitizer.cmake)

add_library(__project_dependencies__ INTERFACE)
add_library(project::dependencies ALIAS __project_dependencies__)

if(USE_CONAN)
	include(cmake/conan.cmake)
	setup_conan(
		REQUIRES
			lua/5.3.5
		SETTINGS
			compiler.libcxx=libstdc++11
		)
	target_link_libraries(__project_dependencies__ INTERFACE
		CONAN_PKG::lua
		)
else()
	find_package(Lua REQUIRED)

	target_link_libraries(__project_dependencies__ INTERFACE
		${LUA_LIBRARIES}
		)
	target_include_directories(__project_dependencies__ INTERFACE
		${LUA_INCLUDE_DIR}
		)
	target_compile_definitions(__project_dependencies__ INTERFACE
		LUA_USE_APICHECK
		)
endif()

include(cmake/build.cmake)
add_subdirectory(src)

include(cmake/tests.cmake)
add_subdirectory(tests)

