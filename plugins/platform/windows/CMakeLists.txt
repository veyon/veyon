INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsCoreFunctions.cpp
	WindowsNetworkFunctions.cpp
	WindowsServiceControl.cpp
	WindowsServiceCore.cpp
	WindowsServiceFunctions.cpp
	WindowsUserFunctions.cpp
	SasEventListener.cpp
	XEventLog.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/authSSP.cpp
	${ultravnc_DIR}/addon/ms-logon/authSSP/GenClientServerContext.cpp
	MOCFILES
	WindowsPlatformPlugin.h
	WindowsCoreFunctions.h
	WindowsNetworkFunctions.h
	WindowsServiceControl.h
	WindowsServiceCore.h
	WindowsServiceFunctions.h
	WindowsUserFunctions.h
	SasEventListener.h
	XEventLog.h
	COTIRE
)

INCLUDE_DIRECTORIES(${ultravnc_DIR}/addon/ms-logon/authSSP)

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
