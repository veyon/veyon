# BuildVeyonApplication.cmake - Copyright (c) 2019-2026 Tobias Junghans
#
# description: build Veyon application
# usage: build_veyon_application(<NAME> <SOURCES>)
include (WindowsBuildHelpers)

if(APPLE)
	option(VEYON_ENABLE_CODESIGN "Enable ad-hoc codesigning of macOS bundles" ON)
	find_program(VEYON_CODESIGN_TOOL codesign)
endif()

function(build_veyon_application TARGET)
	cmake_parse_arguments(PARSE_ARGV 1 arg
		"CONSOLE;NO_BUNDLE;REQUIRE_ADMINISTRATOR_PRIVILEGES"
		"NAME;DESCRIPTION;WINDOWS_ICON"
		"SOURCES")

	if(VEYON_BUILD_ANDROID)
		add_library(${TARGET} SHARED ${arg_SOURCES})
	elseif(APPLE)
		if(arg_NO_BUNDLE)
			add_executable(${TARGET} ${arg_SOURCES})
		else()
			add_executable(${TARGET} MACOSX_BUNDLE ${arg_SOURCES})
		endif()
	else()
		add_executable(${TARGET} ${arg_SOURCES})
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

	if(APPLE)
		if(NOT DEFINED VEYON_VERSION_STRING)
			file(STRINGS "${CMAKE_SOURCE_DIR}/project.yml" _veyon_version_line REGEX "^  version: ")
			string(REGEX REPLACE ".*version:[ ]*" "" VEYON_VERSION_STRING "${_veyon_version_line}")
		endif()
		if(NOT DEFINED VEYON_COPYRIGHT_STRING)
			file(STRINGS "${CMAKE_SOURCE_DIR}/project.yml" _veyon_copyright_line REGEX "^  copyright: ")
			string(REGEX REPLACE ".*copyright:[ ]*" "" VEYON_COPYRIGHT_STRING "${_veyon_copyright_line}")
		endif()

		if(NOT arg_NO_BUNDLE)
			string(TOLOWER "${TARGET}" _veyon_bundle_suffix)
			string(REPLACE "-" "" _veyon_bundle_suffix "${_veyon_bundle_suffix}")
			string(REPLACE "_" "" _veyon_bundle_suffix "${_veyon_bundle_suffix}")
			set(_veyon_bundle_identifier "io.veyon.${_veyon_bundle_suffix}")

			set(_veyon_bundle_version "${VEYON_VERSION_STRING}")
			if(NOT _veyon_bundle_version)
				set(_veyon_bundle_version "1.0")
			endif()

			set_target_properties(${TARGET} PROPERTIES
				MACOSX_BUNDLE TRUE
				MACOSX_BUNDLE_BUNDLE_NAME "${TARGET}"
				MACOSX_BUNDLE_GUI_IDENTIFIER "${_veyon_bundle_identifier}"
				MACOSX_BUNDLE_SHORT_VERSION_STRING "${_veyon_bundle_version}"
				MACOSX_BUNDLE_BUNDLE_VERSION "${_veyon_bundle_version}"
				MACOSX_BUNDLE_COPYRIGHT "© ${VEYON_COPYRIGHT_STRING}"
				MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/MacOSXBundleInfo.plist.in"
			)

			if(VEYON_ENABLE_CODESIGN AND VEYON_CODESIGN_TOOL)
				# Add code signing for macOS (ad-hoc signature for development)
				add_custom_command(TARGET ${TARGET} POST_BUILD
					COMMAND "${VEYON_CODESIGN_TOOL}" --force --deep --sign - "$<TARGET_BUNDLE_DIR:${TARGET}>"
					COMMENT "Signing ${TARGET} with ad-hoc signature"
				)
			elseif(VEYON_ENABLE_CODESIGN)
				message(WARNING "codesign tool not found – skipping ad-hoc signing for target ${TARGET}")
			endif()

			install(TARGETS ${TARGET}
				BUNDLE DESTINATION Applications/Veyon
				RUNTIME DESTINATION bin
			)
		else()
			install(TARGETS ${TARGET}
				RUNTIME DESTINATION bin
			)
		endif()
	else()
		install(TARGETS ${TARGET} RUNTIME DESTINATION bin)
	endif()
endfunction()
