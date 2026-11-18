# BuildVeyonFuzzer.cmake - Copyright (c) 2021-2026 Tobias Junghans
#
# description: build fuzzer test for Veyon component
# usage: build_veyon_fuzzer(<NAME> <SOURCES>)

macro(build_veyon_fuzzer FUZZER_NAME)
	add_executable(${FUZZER_NAME} ${ARGN})
	set_default_target_properties(${FUZZER_NAME})
	target_compile_options(${FUZZER_NAME} PRIVATE "-g;-fsanitize=fuzzer")
	target_link_options(${FUZZER_NAME} PRIVATE "-g;-fsanitize=fuzzer")
	target_link_libraries(${FUZZER_NAME} veyon-core)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/corpus)
		# seed corpus: libFuzzer writes new inputs into the first directory, keep the seeds read-only
		file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/corpus)
		add_test(NAME ${FUZZER_NAME} COMMAND ${FUZZER_NAME} -max_total_time=60
			${CMAKE_CURRENT_BINARY_DIR}/corpus ${CMAKE_CURRENT_SOURCE_DIR}/corpus)
	else()
		add_test(NAME ${FUZZER_NAME} COMMAND ${FUZZER_NAME} -max_total_time=60)
	endif()
	# OpenSSL providers loaded by QCA are never freed (ASan builds)
	set_tests_properties(${FUZZER_NAME} PROPERTIES
		ENVIRONMENT "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/tests/libfuzzer/lsan.supp:print_suppressions=0")
endmacro()

