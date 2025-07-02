# BuildVeyonApplication.cmake - Copyright (c) 2019-2025 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)
include (WindowsBuildHelpers)

function(build_veyon_application TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 arg
		""
		""
		"SOURCES")

	if(VEYON_BUILD_ANDROID)
		add_library(${TARGET} SHARED ${arg_SOURCES})
	else()
		add_executable(${TARGET} ${arg_SOURCES})
		install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
	endif()
	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	set_target_properties(${TARGET} PROPERTIES COMPILE_OPTIONS "${CMAKE_COMPILE_OPTIONS_PIE}")
	set_target_properties(${TARGET} PROPERTIES LINK_OPTIONS "${CMAKE_LINK_OPTIONS_PIE}")
	target_link_libraries(${TARGET} PRIVATE veyon-core)
	set_default_target_properties(${TARGET})
	if(WITH_PCH)
		target_precompile_headers(${TARGET} REUSE_FROM veyon-application-pch)
	endif()
	if (VEYON_BUILD_WINDOWS)
		add_windows_resources(${TARGET} ${ARGN})
	endif()
endfunction()
