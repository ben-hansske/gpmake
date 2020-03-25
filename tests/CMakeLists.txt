cmake_minimum_required(VERSION 3.12)

create_test(
	NAME "test_queue"
	SOURCES
		"test_queue.cpp"
	LIBS
		project::dependencies
	SANITIZE_MEMORY
	# SANITIZE_THREAD
	SANITIZE_UNDEFINED
	)
