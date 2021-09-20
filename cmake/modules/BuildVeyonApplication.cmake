# BuildVeyonApplication.cmake - Copyright (c) 2019-2021 Tobias Junghans
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
	set_property(TARGET ${APPLICATION_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
	set_default_target_properties(${APPLICATION_NAME})

	target_include_directories(${APPLICATION_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_link_libraries(${APPLICATION_NAME} veyon-core)

	if(WITH_PCH)
		target_precompile_headers(${APPLICATION_NAME} REUSE_FROM veyon-pch)
	endif()
endmacro()

