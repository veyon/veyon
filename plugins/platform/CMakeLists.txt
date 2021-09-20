if(VEYON_BUILD_WIN32)
	add_subdirectory(windows)
endif()

if(VEYON_BUILD_LINUX)
	add_subdirectory(linux)
endif()
