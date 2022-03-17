
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
	target_link_libraries(${TARGET_NAME} PUBLIC Qt${QT_MAJOR_VERSION}::Core Qt${QT_MAJOR_VERSION}::Concurrent Qt${QT_MAJOR_VERSION}::Network Qt${QT_MAJOR_VERSION}::Widgets)
	target_precompile_headers(${TARGET_NAME} PUBLIC ${HEADER})
	if(VEYON_BUILD_WINDOWS)
		target_compile_definitions(${TARGET_NAME} PUBLIC _WIN32_WINNT=0x0602)
	endif()
endmacro()
