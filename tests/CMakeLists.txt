if(WITH_FUZZERS)
	add_subdirectory(libfuzzer)
endif()

add_subdirectory(unit)

add_subdirectory(ui)
