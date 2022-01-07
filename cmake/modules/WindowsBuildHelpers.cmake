macro(add_windows_resource TARGET)
	if(VEYON_BUILD_WIN32)
		set(WINRC "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.rc")
		set(RCOBJ "${CMAKE_CURRENT_BINARY_DIR}/winrc-${TARGET}.obj")
		add_custom_command(OUTPUT ${RCOBJ}
			COMMAND ${WINDRES}
			-I${CMAKE_CURRENT_SOURCE_DIR}
			-o${RCOBJ}
			-i${WINRC}
			DEPENDS ${WINRC})
		target_sources(${TARGET} PUBLIC ${RCOBJ})
	endif()
endmacro()

macro(make_graphical_app TARGET)
	if(VEYON_BUILD_WINDOWS)
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS -mwindows)
	endif()
endmacro()

macro(make_console_app TARGET)
	if(VEYON_BUILD_WINDOWS)
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS -mconsole)
	endif()
endmacro()
