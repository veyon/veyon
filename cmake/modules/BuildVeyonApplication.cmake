# BuildVeyonApplication.cmake - Copyright (c) 2019-2023 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)

macro(build_veyon_application APPLICATION_NAME)
	if(VEYON_BUILD_ANDROID)
		add_library(${APPLICATION_NAME} SHARED ${ARGN})
	else()
		add_executable(${APPLICATION_NAME} ${ARGN})
		install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION bin)
	endif()
	target_include_directories(${APPLICATION_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	set_target_properties(${APPLICATION_NAME} PROPERTIES COMPILE_OPTIONS "${CMAKE_COMPILE_OPTIONS_PIE}")
	set_target_properties(${APPLICATION_NAME} PROPERTIES LINK_OPTIONS "${CMAKE_LINK_OPTIONS_PIE}")
	target_link_libraries(${APPLICATION_NAME} PRIVATE veyon-core)
	set_default_target_properties(${APPLICATION_NAME})
	if(WITH_PCH)
		target_precompile_headers(${APPLICATION_NAME} REUSE_FROM veyon-application-pch)
	endif()
endmacro()

