# BuildApplication.cmake - Copyright (c) 2019 Tobias Junghans
#
# description: build Veyon application
# usage: build_application(<NAME> <SOURCES>)

macro(BUILD_APPLICATION APPLICATION_NAME)
	set(APPLICATION_SOURCES ${ARGN})

	if(VEYON_BUILD_ANDROID)
		add_library(${APPLICATION_NAME} SHARED ${APPLICATION_SOURCES})
	else()
		add_executable(${APPLICATION_NAME} ${APPLICATION_SOURCES})
		install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION bin)
	endif()
	target_include_directories(${APPLICATION_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_link_libraries(${APPLICATION_NAME} veyon-core)
	target_compile_options(${APPLICATION_NAME} PRIVATE ${VEYON_COMPILE_OPTIONS})
	set_default_target_properties(${APPLICATION_NAME})
endmacro()

