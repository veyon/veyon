file(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

foreach(plugin ${plugins})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${plugin})
		add_subdirectory(${plugin})
	endif()
endforeach()
