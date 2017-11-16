INCLUDE(BuildPlugin)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsCoreFunctions.cpp
	WindowsNetworkFunctions.cpp
	WindowsUserInfoFunctions.cpp
	MOCFILES
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsNetworkFunctions.h
	WindowsUserInfoFunctions.h)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
