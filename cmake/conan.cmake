cmake_minimum_required(VERSION 3.12)

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
    file(DOWNLOAD
        "https://raw.githubusercontent.com/conan-io/cmake-conan/release/0.15/conan.cmake"
        "${CMAKE_BINARY_DIR}/conan.cmake"
        )
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

function(setup_conan)
    conan_cmake_run(
        ${ARGV}
        BASIC_SETUP
        CMAKE_TARGETS
        )
endfunction()
