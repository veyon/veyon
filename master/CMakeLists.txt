include(BuildVeyonApplication)

file(GLOB master_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.*)

set(master_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/resources/master.qrc)
if(VEYON_DEBUG)
	set(master_RESOURCES ${master_RESOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/resources/examples.qrc)
endif()

build_veyon_application(veyon-master
	NAME Master
	WINDOWS_ICON data/veyon-master.ico
	SOURCES
	${master_SOURCES}
	${master_RESOURCES})

if(VEYON_BUILD_LINUX)
	xdg_install(${CMAKE_CURRENT_BINARY_DIR}/data/veyon-master.desktop ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.xpm ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.png ${CMAKE_CURRENT_SOURCE_DIR}/data/veyon-master.svg)
endif()

if(APPLE)
	set(_veyon_macos_helpers
		veyon-auth-helper
		veyon-cli
		veyon-server
		veyon-service
		veyon-worker
	)
	foreach(_helper ${_veyon_macos_helpers})
		if(TARGET ${_helper})
			add_dependencies(veyon-master ${_helper})
			if(_helper STREQUAL "veyon-auth-helper")
				set(_helper_mode 4755)
			else()
				set(_helper_mode 755)
			endif()
			add_custom_command(TARGET veyon-master POST_BUILD
				COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${_helper}>" "$<TARGET_FILE_DIR:veyon-master>/${_helper}"
				COMMAND /bin/chmod ${_helper_mode} "$<TARGET_FILE_DIR:veyon-master>/${_helper}"
				COMMENT "Embedding ${_helper} into veyon-master bundle"
				VERBATIM
			)
		endif()
	endforeach()
	unset(_helper_mode)
	unset(_veyon_macos_helpers)
endif()
