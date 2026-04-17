macro(set_default_target_properties TARGET_NAME)
	set_property(TARGET ${TARGET_NAME} PROPERTY NO_SYSTEM_FROM_IMPORTED ON)
	set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD 20)
	set_property(TARGET ${TARGET_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
	target_compile_definitions(${TARGET_NAME} PRIVATE
		-DQT_DEPRECATED_WARNINGS
		-DQT_DISABLE_DEPRECATED_BEFORE=0x050e00
		-DQT_NO_CAST_FROM_ASCII
		-DQT_NO_CAST_TO_ASCII
		-DQT_NO_CAST_FROM_BYTEARRAY
		-DQT_NO_KEYWORDS
		-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT
		-DQT_USE_QSTRINGBUILDER
		-DQT_STRICT_ITERATORS
	)
	target_compile_options(${TARGET_NAME} PRIVATE "-Wall;-Werror")
	if(WITH_LTO)
		if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.13.0")
			target_compile_options(${TARGET_NAME} PRIVATE ${GCC_LTO_FLAGS})
			target_link_options(${TARGET_NAME} PRIVATE ${GCC_LTO_FLAGS})
		else()
			set_target_properties(${TARGET_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
		endif()
	endif()
	if(WITH_ADDRESS_SANITIZER)
		target_link_options(${TARGET_NAME} PRIVATE "-static-libsan")
		target_compile_options(${TARGET_NAME} PRIVATE "-fsanitize=address")
		target_link_options(${TARGET_NAME} PRIVATE "-fsanitize=address")
	else()
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-Wl,-no-undefined")
	endif()
	if(WITH_THREAD_SANITIZER)
		target_compile_options(${TARGET_NAME} PRIVATE "-fsanitize=thread")
		target_link_options(${TARGET_NAME} PRIVATE "-fsanitize=thread")
	endif()
	if(WITH_UB_SANITIZER)
		target_compile_options(${TARGET_NAME} PRIVATE "-fsanitize=undefined")
		target_link_options(${TARGET_NAME} PRIVATE "-fsanitize=undefined")
	endif()
endmacro()
