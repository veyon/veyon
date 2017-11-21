INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsCoreFunctions.cpp
	WindowsNetworkFunctions.cpp
	WindowsService.cpp
	WindowsServiceFunctions.cpp
	WindowsUserInfoFunctions.cpp
	MOCFILES
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsNetworkFunctions.h
	WindowsService.h
	WindowsServiceFunctions.h
	WindowsUserInfoFunctions.h
	COTIRE
)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
