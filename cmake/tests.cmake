cmake_minimum_required(VERSION 3.12)

include(CTest)

set(COMPILER_ID "GNU")
if(CMAKE_C_COMPILER_ID AND CMAKE_CXX_COMPILER_ID)
    if(${CMAKE_C_COMPILER_ID} STREQUAL ${CMAKE_CXX_COMPILER_ID})
        set(COMPILER_ID CMAKE_CXX_COMPILER_ID)
    else()
        message(WARNING "C-Compiler does not fit C++-Compiler")
    endif()
elseif(CMAKE_C_COMPILER_ID)
    set(COMPILER_ID ${CMAKE_C_COMPILER_ID})
elseif(CMAKE_CXX_COMPILER_ID)
    set(COMPILER_ID ${CMAKE_CXX_COMPILER_ID})
else()
    message(WARNING "Language not supported")
endif()

if(COMPILER_ID STREQUAL "Clang")
	set(SANITIZER_LIST_MEMORY
		"address"
		"memory"
		"safe-stack"
		)
	set(SANITIZER_LIST_THREAD
		"thread"
		)
	set(SANITIZER_LIST_UNDEFINED
		"undefined"
		)
elseif(COMPILER_ID STREQUAL "GNU")
	set(SANITIZER_LIST_MEMORY
		"address"
		# "kernel-address"
		"address,pointer-compare"
		"address,pointer-subtract"
		"leak"
		)
	set(SANITIZER_LIST_THREAD
		"thread"
		)
	set(SANITIZER_LIST_UNDEFINED
		"undefined"
		)
endif()

function(create_test)
	cmake_parse_arguments(CT 
		"SANITIZE_MEMORY;SANITIZE_THREAD;SANITIZE_UNDEFINED"
		"NAME"
        "SOURCES;LIBS;DEFINITIONS"
        ${ARGN}
        )
	add_library("${CT_NAME}_LIB" INTERFACE)
	set(PROJECT_TARGETS)
	if(TARGET project::config)
		target_link_libraries("${CT_NAME}_LIB" INTERFACE
			project::config
			)
	endif()
	target_link_libraries("${CT_NAME}_LIB" INTERFACE
		${CT_LIBS}
		)
	target_compile_definitions("${CT_NAME}_LIB" INTERFACE
		${CT_DEFINITIONS}
		)
	target_include_directories("${CT_NAME}_LIB" INTERFACE
		${CMAKE_SOURCE_DIR}/src
		)

	if(NOT CT_SANITIZE_MEMORY AND NOT CT_SANITIZE_THREAD AND NOT CT_SANITIZE_UNDEFINED)
		add_executable("${CT_NAME}"
			${CT_SOURCES}
			)
		if(TARGET project::sanitizer)
			target_link_libraries("${CT_NAME}" PUBLIC
				project::sanitizer
				)
		endif()
		target_link_libraries("${CT_NAME}" PUBLIC "${CT_NAME}_LIB")
		add_test("${CT_NAME}" "${CT_NAME}")
	else()
		if(CT_SANITIZE_MEMORY)
			foreach(SANITIZER_GROUP IN LISTS SANITIZER_LIST_MEMORY)
				string(REPLACE "," "_" NAME_SUFFIX ${SANITIZER_GROUP})
				set(EXE_NAME "${CT_NAME}_${NAME_SUFFIX}")
				add_executable("${EXE_NAME}"
					"${CT_SOURCES}"
					)
				target_link_libraries("${EXE_NAME}" PUBLIC
					"${CT_NAME}_LIB"
					"-fsanitize=${SANITIZER_GROUP}"
					)
				target_compile_options("${EXE_NAME}" PUBLIC
					"-fsanitize=${SANITIZER_GROUP}"
					)
				add_test("${EXE_NAME}" "${EXE_NAME}")
			endforeach()
		endif()
		if(CT_SANITIZE_THREAD)
			foreach(SANITIZER_GROUP IN LISTS SANITIZER_LIST_THREAD)
				string(REPLACE "," "_" NAME_SUFFIX ${SANITIZER_GROUP})
				set(EXE_NAME "${CT_NAME}_${NAME_SUFFIX}")
				add_executable("${EXE_NAME}"
					"${CT_SOURCES}"
					)
				target_link_libraries("${EXE_NAME}" PUBLIC
					"${CT_NAME}_LIB"
					"-fsanitize=${SANITIZER_GROUP}"
					)
				target_compile_options("${EXE_NAME}" PUBLIC
					"-fsanitize=${SANITIZER_GROUP}"
					)
				add_test("${EXE_NAME}" "${EXE_NAME}")
			endforeach()
		endif()
		if(CT_SANITIZE_UNDEFINED)
			foreach(SANITIZER_GROUP IN LISTS SANITIZER_LIST_UNDEFINED)
				string(REPLACE "," "_" NAME_SUFFIX ${SANITIZER_GROUP})
				set(EXE_NAME "${CT_NAME}_${NAME_SUFFIX}")
				add_executable("${EXE_NAME}"
					"${CT_SOURCES}"
					)
				target_link_libraries("${EXE_NAME}" PUBLIC
					"${CT_NAME}_LIB"
					"-fsanitize=${SANITIZER_GROUP}"
					)
				target_compile_options("${EXE_NAME}" PUBLIC
					"-fsanitize=${SANITIZER_GROUP}"
					)
				add_test("${EXE_NAME}" "${EXE_NAME}")
			endforeach()
		endif()
	endif()
endfunction()
