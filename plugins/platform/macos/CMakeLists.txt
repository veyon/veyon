add_subdirectory(auth-helper)

find_package(PkgConfig QUIET)

include(BuildVeyonPlugin)


build_veyon_plugin(macos-platform
	MacOsPlatformPlugin.cpp
	MacOsCoreFunctions.cpp
	MacOsPlatformConfigurationPage.h
	MacOsPlatformConfigurationPage.cpp
	MacOsPlatformConfigurationPage.ui
	MacOsFilesystemFunctions.cpp
	MacOsInputDeviceFunctions.cpp
	MacOsNetworkFunctions.cpp
	MacOsServiceFunctions.cpp
	MacOsServiceCore.cpp
	MacOsSessionFunctions.cpp
	MacOsUserFunctions.cpp
	MacOsPlatformPlugin.h
	MacOsCoreFunctions.h
	MacOsFilesystemFunctions.h
	MacOsInputDeviceFunctions.h
	MacOsKeyboardShortcutTrapper.h
	MacOsNetworkFunctions.h
	MacOsServiceFunctions.h
	MacOsServiceCore.h
	MacOsSessionFunctions.h
	MacOsUserFunctions.h
	macos.qrc
	../common/LogonHelper.h
	../common/LogonHelper.cpp
	../common/PersistentLogonCredentials.h
	../common/PersistentLogonCredentials.cpp
	../common/PlatformSessionManager.h
	../common/PlatformSessionManager.cpp
	../common/ServiceDataManager.h
	../common/ServiceDataManager.cpp
)

target_include_directories(macos-platform PRIVATE
	../common
)

target_link_libraries(macos-platform
	)

