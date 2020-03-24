cmake_minimum_required(VERSION 3.12)

if(CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "Debug")
endif()
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project settings
add_library(__project_config__ INTERFACE)
add_library(project::config ALIAS __project_config__)

function(setup_config standard)
    # Compiler warnings
    if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang") # enable Clang-warnings
        set(CLANG_WARNINGS
			-Weverything
			-Wno-c++98-compat
			-Wno-c++98-compat-pedantic
			-Wno-missing-prototypes
			-Wno-ctad-maybe-unsupported
            )
        target_compile_options(__project_config__ INTERFACE
            ${CLANG_WARNINGS}
            )
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(GCC_WARNINGS
            -Wall
            -Wextra
            )
        target_compile_options(__project_config__ INTERFACE
            ${GCC_WARNINGS}
            )
    else()
        if(CMAKE_C_COMPILER_ID)
            message(WARNING "Compiler ${CMAKE_C_COMPILER_ID} not supported.")
        else()
            message(WARNING "Compiler ${CMAKE_CXX_COMPILER_ID} not supported.")
        endif()
    endif()

    target_compile_features(__project_config__ INTERFACE
        ${standard}
        )
endfunction()
