cmake_minimum_required(VERSION 3.12)

include(CTest)

function(create_test)
    cmake_parse_arguments(CT "" "NAME"
        "SOURCES;LIBS;DEFINITIONS"
        ${ARGN}
        )
    add_executable("${CT_NAME}"
        ${CT_SOURCES}
        )

    set(PROJECT_TARGETS)
    if(TARGET project::config)
        set(PROJECT_TARGETS project::config)
    endif()
    if(TARGET project::sanitizer)
        set(PROJECT_TARGETS ${PROJECT_TARGETS} project::sanitizer)
    endif()

    target_link_libraries("${CT_NAME}"
        PUBLIC
            ${PROJECT_TARGETS}
            ${CT_LIBS}
        )
    target_compile_definitions("${CT_NAME}" PRIVATE
        ${CT_DEFINITIONS}
        )
    add_test("${CT_NAME}" "${CT_NAME}")
endfunction()
