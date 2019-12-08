macro(ADD_WINDOWS_RESOURCE TARGET)
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

macro(MAKE_GRAPHICAL_APP TARGET)
	if(VEYON_BUILD_WIN32)
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS -mwindows)
	endif()
endmacro()

macro(MAKE_CONSOLE_APP TARGET)
	if(VEYON_BUILD_WIN32)
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS -mconsole)
	endif()
endmacro()
