add_subdirectory(auth-helper)
# NOTE: service-helper subdirectory removed - service installation handled by external script

include(BuildVeyonPlugin)

build_veyon_plugin(osx-platform
	NAME OsXPlatform
	SOURCES
	OsXPlatformPlugin.cpp
	OsXCoreFunctions.cpp
	OsXFilesystemFunctions.cpp
	OsXInputDeviceFunctions.cpp
	OsXNetworkFunctions.cpp
	OsXServiceFunctions.cpp
	OsXSessionFunctions.cpp
	OsXUserFunctions.cpp
	OsXPlatformPlugin.h
	OsXCoreFunctions.h
	OsXFilesystemFunctions.h
	OsXInputDeviceFunctions.h
	OsXKeyboardShortcutTrapper.h
	OsXNetworkFunctions.h
	OsXServiceFunctions.h
	OsXSessionFunctions.h
	OsXUserFunctions.h
	)

target_link_libraries(osx-platform PRIVATE
	Qt${QT_MAJOR_VERSION}::Core
	Qt${QT_MAJOR_VERSION}::Gui
	Qt${QT_MAJOR_VERSION}::Network
	)
