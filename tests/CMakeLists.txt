if(WITH_FUZZERS)
	add_subdirectory(libfuzzer)
endif()

add_subdirectory(unit)

if(WITH_TESTS)
	add_subdirectory(cmake)
endif()
