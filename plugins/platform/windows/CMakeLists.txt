INCLUDE(BuildPlugin)

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
	WindowsUserInfoFunctions.h)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
