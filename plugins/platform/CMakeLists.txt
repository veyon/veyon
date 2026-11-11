if(VEYON_BUILD_WINDOWS)
	add_subdirectory(windows)
endif()

if(VEYON_BUILD_LINUX)
	add_subdirectory(linux)
endif()

if(VEYON_BUILD_APPLE)
	add_subdirectory(osx)
endif()
