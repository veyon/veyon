# BuildVeyonFuzzer.cmake - Copyright (c) 2021-2025 Tobias Junghans
#
# description: build fuzzer test for Veyon component
# usage: build_veyon_fuzzer(<NAME> <SOURCES>)

macro(build_veyon_fuzzer FUZZER_NAME)
	add_executable(${FUZZER_NAME} ${ARGN})
	set_default_target_properties(${FUZZER_NAME})
	target_compile_options(${FUZZER_NAME} PRIVATE "-g;-fsanitize=fuzzer")
	target_link_options(${FUZZER_NAME} PRIVATE "-g;-fsanitize=fuzzer")
	target_link_libraries(${FUZZER_NAME} veyon-core)
	add_test(NAME ${FUZZER_NAME} COMMAND ${FUZZER_NAME} -max_total_time=60)
endmacro()

