# BuildVeyonApplication.cmake - Copyright (c) 2019-2020 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)


macro(build_veyon_application APPLICATION_NAME)
	if(VEYON_BUILD_ANDROID)
		add_library(${APPLICATION_NAME} SHARED ${ARGN})
	elseif(VEYON_BUILD_APPLE)
		if (VEYON_BUILD_APPLE_BUNDLE)
			set(MACOSX_BUNDLE_ICON_FILE ${APPLICATION_NAME}.icns)
			set(MACOSX_BUNDLE_GUI_IDENTIFIER "io.veyon.${APPLICATION_NAME}")
			set(MACOSX_BUNDLE_INFO_STRING "Cross-platform computer monitoring and classroom management")
			set(VEYON_APPLE_ICON ${CMAKE_CURRENT_SOURCE_DIR}/data/${APPLICATION_NAME}.icns)
			set_source_files_properties(
					${VEYON_APPLE_ICON} PROPERTIES
					MACOSX_PACKAGE_LOCATION "Resources"
			)
			add_executable(${APPLICATION_NAME} MACOSX_BUNDLE ${VEYON_APPLE_ICON} ${ARGN})
			install(TARGETS ${APPLICATION_NAME} BUNDLE DESTINATION ${VEYON_APPLE_INSTALL_DIR})
		else()
			add_executable(${APPLICATION_NAME} ${ARGN})
			install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION bin)
		endif ()
	else()
		add_executable(${APPLICATION_NAME} ${ARGN})
		install(TARGETS ${APPLICATION_NAME} RUNTIME DESTINATION bin)
	endif()
	target_include_directories(${APPLICATION_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)
	target_link_libraries(${APPLICATION_NAME} veyon-core)
	target_compile_options(${APPLICATION_NAME} PRIVATE ${VEYON_COMPILE_OPTIONS})
	set_property(TARGET ${APPLICATION_NAME} PROPERTY POSITION_INDEPENDENT_CODE TRUE)
	set_default_target_properties(${APPLICATION_NAME})
endmacro()

