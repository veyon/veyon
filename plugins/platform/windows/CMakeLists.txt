INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsCoreFunctions.cpp
	WindowsNetworkFunctions.cpp
	WindowsServiceControl.cpp
	WindowsServiceCore.cpp
	WindowsServiceFunctions.cpp
	WindowsUserInfoFunctions.cpp
	SasEventListener.cpp
	MOCFILES
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsNetworkFunctions.h
	WindowsServiceControl.h
	WindowsServiceCore.h
	WindowsServiceFunctions.h
	WindowsUserInfoFunctions.h
	SasEventListener.h
	COTIRE
)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
