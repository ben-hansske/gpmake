cmake_minimum_required(VERSION 3.12)

function(create_exe)
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
endfunction()

function(create_slib)
    cmake_parse_arguments(CT "" "NAME"
        "SOURCES;LIBS;DEFINITIONS"
        ${ARGN}
        )
    add_library("${CT_NAME}" STATIC
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
endfunction()

function(create_dlib)
    cmake_parse_arguments(CT "" "NAME"
        "SOURCES;LIBS;DEFINITIONS"
        ${ARGN}
        )
    add_library("${CT_NAME}" SHARED
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
endfunction()


function(create_header_lib)
	cmake_parse_arguments(CT "" "NAME"
		"SOURCES;LIBS;DIRS"
        ${ARGN}
        )
	add_custom_target("${CT_NAME}_HEADERS" SOURCES
		${CT_COURCES}
		)
    add_library("${CT_NAME}" INTERFACE)
	set(PROJECT_TARGETS)
    if(TARGET project::config)
        set(PROJECT_TARGETS project::config)
    endif()
    if(TARGET project::sanitizer)
        set(PROJECT_TARGETS ${PROJECT_TARGETS} project::sanitizer)
    endif()

    target_link_libraries("${CT_NAME}" INTERFACE
        ${PROJECT_TARGETS}
        ${CT_LIBS}
        )
    target_include_directories("${CT_NAME}" INTERFACE
		${CT_DIRS}
		)
endfunction()
