# BuildVeyonPlugin.cmake - Copyright (c) 2017-2026 Tobias Junghans
#
# description: build Veyon plugin
# usage: build_veyon_plugin(<NAME> <SOURCES>)

include(SetDefaultTargetProperties)
include(WindowsBuildHelpers)

set(VEYON_PLUGIN_BUILD_VERSION_TEMPLATE "${CMAKE_CURRENT_LIST_DIR}/VeyonPluginBuildVersion.cpp.in")

# Adds the function veyonPluginBuildVersion() to a plugin. It returns the Veyon version the plugin
# has been built for, which PluginManager checks before instantiating the plugin. There's no ABI
# compatibility between Veyon releases, so plugins built for any other version are refused.
# The version can only be overridden for tests.
function(add_veyon_plugin_build_version TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 arg "" "VERSION" "")
	if(arg_VERSION)
		set(VEYON_PLUGIN_BUILD_VERSION "${arg_VERSION}")
	else()
		set(VEYON_PLUGIN_BUILD_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
	endif()
	set(BUILD_VERSION_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}-build-version.cpp")
	configure_file(${VEYON_PLUGIN_BUILD_VERSION_TEMPLATE} ${BUILD_VERSION_SOURCE} @ONLY)
	set_source_files_properties(${BUILD_VERSION_SOURCE} PROPERTIES
		SKIP_UNITY_BUILD_INCLUSION TRUE
		SKIP_PRECOMPILE_HEADERS TRUE)
	target_sources(${TARGET} PRIVATE ${BUILD_VERSION_SOURCE})
endfunction()

function(build_veyon_plugin TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 arg
		""
		""
		"SOURCES")

	set(LIBRARY_TYPE "MODULE")
	if(WITH_TESTS)
		set(LIBRARY_TYPE "SHARED")
	endif()
	add_library(${TARGET} ${LIBRARY_TYPE} ${arg_SOURCES})
	add_veyon_plugin_build_version(${TARGET})

	target_include_directories(${TARGET} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
	target_link_libraries(${TARGET} PRIVATE veyon-core)

	set_default_target_properties(${TARGET})
	set_target_properties(${TARGET} PROPERTIES PREFIX "")
	if(${TARGET}_COMPONENT)
		install(TARGETS ${TARGET} COMPONENT ${${TARGET}_COMPONENT} LIBRARY DESTINATION ${VEYON_INSTALL_PLUGIN_DIR})
	else()
		install(TARGETS ${TARGET} LIBRARY DESTINATION ${VEYON_INSTALL_PLUGIN_DIR})
	endif()
	if(WITH_PCH)
		target_precompile_headers(${TARGET} REUSE_FROM veyon-library-pch)
	endif()
	if (VEYON_BUILD_WINDOWS)
		add_windows_resources(${TARGET} ${ARGN})
	endif()
endfunction()

macro(test_veyon_plugin TARGET TEST_NAME)
	if(WITH_TESTS)
		add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
		add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
		target_link_libraries(${TEST_NAME} PRIVATE veyon-core ${TARGET})
	endif()
endmacro()
