
macro(add_pch_target TARGET_NAME HEADER)
	add_library(${TARGET_NAME} STATIC ${HEADER})

	if(${CMAKE_VERSION} VERSION_GREATER "3.17.5")
		file(GENERATE
			OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/empty_pch.cxx
			CONTENT "/*empty file*/")
		set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/empty_pch.cxx PROPERTIES SKIP_UNITY_BUILD_INCLUSION TRUE SKIP_AUTOGEN TRUE)
		target_sources(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}/empty_pch.cxx)
	endif()
	set_default_target_properties(${TARGET_NAME})
	if(WITH_QT6)
		target_link_libraries(${TARGET_NAME} PUBLIC Qt6::Core Qt6::Concurrent Qt6::Network Qt6::Widgets)
	else()
		target_link_libraries(${TARGET_NAME} PUBLIC Qt5::Core Qt5::Concurrent Qt5::Network Qt5::Widgets)
	endif()
	target_precompile_headers(${TARGET_NAME} PUBLIC ${HEADER})
endmacro()
