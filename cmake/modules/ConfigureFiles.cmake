macro(configure_files)
	foreach(f ${ARGN})
		string(REPLACE ".in" "" OUT_FILE "${f}")
		configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${f} ${CMAKE_CURRENT_BINARY_DIR}/${OUT_FILE} @ONLY)
	endforeach()
endmacro()

