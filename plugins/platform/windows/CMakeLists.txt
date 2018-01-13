INCLUDE(BuildPlugin)

ADD_DEFINITIONS(-DUNICODE -D_UNICODE -D_WIN32_WINNT=0x0600)

BUILD_PLUGIN(windows-platform
	WindowsPlatformPlugin.cpp
	WindowsCoreFunctions.cpp
	WindowsInputDeviceFunctions.cpp
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
	WindowsInputDeviceFunctions.h
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
INCLUDE_DIRECTORIES(${Qt5Gui_PRIVATE_INCLUDE_DIRS})

TARGET_LINK_LIBRARIES(windows-platform -lwtsapi32)
